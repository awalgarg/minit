#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>

#define MINITROOT "/etc/minit"

#undef printf
extern int printf(const char *format,...);

extern int openreadclose(char *fn, char **buf, unsigned long *len);
extern char **split(char *buf,int c,int *len,int plus,int ofs);

extern char **environ;

static struct process {
  char *name;
/*  char **argv; */
  pid_t pid;
  char respawn;
  char circular;
  time_t startedat;
} *root;

static int maxprocess=-1;

static int processalloc=0;

/* return index of service in process data structure or -1 if not found */
int findservice(char *service) {
  int i;
  for (i=0; i<=maxprocess; i++) {
    if (!strcmp(root[i].name,service))
      return i;
  }
  return -1;
}

/* look up process index in data structure by PID */
int findbypid(pid_t pid) {
  int i;
  for (i=0; i<=maxprocess; i++) {
    if (root[i].pid == pid)
      return i;
  }
  return -1;
}

/* clear circular dependency detection flags */
void circsweep() {
  int i;
  for (i=0; i<=maxprocess; i++)
    root[i].circular=0;
}

/* add process to data structure, return index or -1 */
int addprocess(struct process *p) {
  if (maxprocess+1>=processalloc) {
    struct process *fump;
    processalloc+=8;
    if ((fump=(struct process *)realloc(root,processalloc*sizeof(struct process)))==0) return -1;
    root=fump;
  }
  memmove(&root[++maxprocess],p,sizeof(struct process));
  return maxprocess;
}

/* load a service into the process data structure and return index or -1
 * if failed */
int loadservice(char *service) {
  struct process tmp;
  int fd;
  if (*service==0) return 0;
  fd=findservice(service);
  if (fd>=0) return fd;
  if (chdir(MINITROOT) || chdir(service)) return -1;
  if (!(tmp.name=strdup(service))) return -1;
  tmp.pid=0;
  fd=open("respawn",O_RDONLY);
  if (fd>=0) {
    tmp.respawn=1;
    close(fd);
  } else
    tmp.respawn=0;
  tmp.startedat=0;
  tmp.circular=0;
  addprocess(&tmp);
}

/* usage: isup(findservice("sshd")).
 * returns nonzero if process is up */
int isup(int service) {
  if (service<0) return 0;
  return (root[service].pid!=0);
}

/* called from inside the service directory, return the PID or 0 on error */
pid_t forkandexec(int pause) {
  char **argv=0;
  int count=0;
  pid_t p;
  int fd;
  unsigned long len;
  char *s=0;
  int argc;
  char *argv0=0;

again:
  switch (p=fork()) {
  case -1:
    if (count>3) return 0;
    sleep(++count*2);
    goto again;
  case 0:
    /* child */
    if (pause) {
      struct timespec req;
      req.tv_sec=0;
      req.tv_nsec=500000000;
      nanosleep(&req,0);
    }
    if (!openreadclose("params",&s,&len)) {
      argv=split(s,'\n',&argc,2,1);
      if (argv[argc-1]) argv[argc-1]=0; else argv[argc]=0;
    } else {
      argv=(char**)malloc(2*sizeof(char*));
      argv[1]=0;
    }
    argv0=(char*)malloc(PATH_MAX+1);
    if (!argv || !argv0) goto abort;
    if (readlink("run",argv0,PATH_MAX)<0) {
      if (errno!=EINVAL) goto abort;	/* not a symbolic link */
      argv0=strdup("./run");
    }
/*    chdir("/"); */
    argv[0]=strrchr(argv0,'/');
    if (argv[0])
      argv[0]++;
    else
      argv[0]=argv0;
    execve("run",argv,environ);
    _exit(0);
  abort:
    free(argv0);
    free(argv);
    return 0;
  default:
    fd=open("sync",O_RDONLY);
    if (fd>=0) {
      close(fd);
      waitpid(p,0,0);
    }
    return p;
  }
}

/* start a service, return nonzero on error */
int startnodep(int service,int pause) {
  pid_t p;
  /* step 1: see if the process is already up */
  if (isup(service)) return 0;
#if 0
  printf("launching %s\n",root[service].name);
#endif
  /* step 2: fork and exec service, put PID in data structure */
  if (chdir(MINITROOT) || chdir(root[service].name)) return -1;
  root[service].startedat=time(0);
  root[service].pid=forkandexec(pause);
  return root[service].pid;
}

int startservice(int service,int pause) {
  int dir=-1,fd;
  unsigned long len;
  char *s=0;
  pid_t pid;
  if (service<0) return 0;
#if 0
  write(1,"startservice ",13);
  write(1,root[service].name,strlen(root[service].name));
  write(1,"\n",1);
#endif
  if (chdir(MINITROOT) || chdir(root[service].name)) return -1;
  if (root[service].circular)
    return 0;
  root[service].circular=1;
  if ((dir=open(".",O_RDONLY))>=0) {
    if (!openreadclose("depends",&s,&len)) {
      char **argv;
      int argc,i;
      argv=split(s,'\n',&argc,0,0);
      for (i=0; i<argc; i++)
	startservice(loadservice(argv[i]),0);
      fchdir(dir);
    }
    pid=startnodep(service,pause);

#if 0
    write(1,"started service ",17);
    write(1,root[service].name,strlen(root[service].name));
    write(1," -> ",4);
    {
      char buf[10];
      snprintf(buf,10,"%d\n",pid);
      write(1,buf,strlen(buf));
    }
#endif
    close(dir);
    dir=-1;
    return pid;
  }
  return 0;
}

void sulogin() {	/* exiting on an initialization failure is not a good idea for init */
  char *argv[]={"sulogin",0};
  execve("/sbin/sulogin",argv,environ);
  exit(1);
}

void childhandler() {
  int status,i;
  pid_t killed;
#undef debug
#ifdef debug
  write(2,"wait...",7);
#endif
  killed=waitpid(-1,&status,WNOHANG);
#ifdef debug
  {
    char buf[50];
    snprintf(buf,50," %d\n",killed);
    write(2,buf,strlen(buf));
  }
#endif
  if (killed == (pid_t)-1) {
    write(2,"all services exited.\n",21);
    exit(0);
  }
  if (killed==0) return;
  i=findbypid(killed);
#if 0
  printf("%d exited, idx %d -> service %s\n",killed,i,i>=0?root[i].name:"[unknown]");
#endif
  if (i>=0) {
    root[i].pid=0;
    if (root[i].respawn) {
#if 0
      printf("restarting %s\n",root[i].name);
#endif
      circsweep();
      startservice(i,time(0)-root[i].startedat<1);
    } else
      root[i].pid=1;
  }
}

static volatile int dowait=0;

void sigchild(int whatever) { dowait=1; }

main(int argc, char *argv[]) {
  /* Schritt 1: argv[1] als Service nehmen und starten */
  int count=0;
  int i;
  int infd=open("/etc/minit/in",O_RDWR),outfd=open("/etc/minit/out",O_RDWR|O_NONBLOCK);
  struct pollfd pfd;
/*  int s=bindsocket(); */
/*  printf("%d %d\n",infd,outfd);
  exit(0); */
  int nfds=1;

  signal(SIGCHLD,sigchild);
  if (infd<0 || outfd<0) {
    puts("minit: could not open /etc/minit/in or /etc/minit/out\n");
    nfds=0;
  } else
    pfd.fd=infd;
  pfd.events=POLLIN;

  for (i=1; i<argc; i++) {
    circsweep();
    if (startservice(loadservice(argv[i]),0)) count++;
  }
  circsweep();
  if (!count) startservice(loadservice("default"),0);
  for (;;) {
    int status;
    int i;
    char buf[1501];
/*    if (dowait) {
      dowait=0; */
      childhandler();
/*    } */
    switch (poll(&pfd,nfds,500)) {
    case -1:
      if (errno==EINTR) {
	childhandler();
	break;
      }
      puts("poll failed!\n");
      sulogin();
      /* what should we do if poll fails?! */
      break;
    case 1:
      i=read(infd,buf,1500);
      if (i>1) {
	pid_t pid;
	int idx,tmp;
	buf[i]=0;

/*	write(1,buf,strlen(buf)); write(1,"\n",1); */
	if (buf[0]!='s' && ((idx=findservice(buf+1))<0))
error:
	  write(outfd,"0",1);
	else {
	  switch (buf[0]) {
	  case 'p':
	    i=snprintf(buf,10,"%d",root[idx].pid);
	    write(outfd,buf,strlen(buf));
	    break;
	  case 'r':
	    root[idx].respawn=0;
	    goto ok;
	  case 'R':
	    root[idx].respawn=1;
	    goto ok;
	  case 'P':
	    tmp=strtol(buf+strlen(buf)+1,0);
	    if (tmp>0) pid=tmp;
	    root[idx].pid=tmp;
	    goto ok;
	  case 's':
	    idx=loadservice(buf+1);
	    if (idx<0) goto error;
	    if (root[idx].pid<2) {
	      root[idx].pid=0;
	      circsweep();
	      idx=startservice(idx,0);
	      if (idx==0) {
		write(outfd,"0",1);
		break;
	      }
	    }
ok:
	    write(outfd,"1",1);
	    break;
	  case 'u':
	    snprintf(buf,10,"%d",time(0)-root[idx].startedat);
	    write(outfd,buf,strlen(buf));
	  }
	}
      }
      break;
    default:
    }
  }
}