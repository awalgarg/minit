#include <fcntl.h>
#include <sys/file.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "str.h"
#include "fmt.h"
#include "buffer.h"
#define NOVARS
#include "minit.h"

static int infd,outfd;

static char buf[1500];

void addservice(char* service) {
  char* x;
  if (str_start(service,MINITROOT "/"))
    service+=sizeof(MINITROOT "/") -1;
  x=service+str_len(service)-1;
  while (x>service && *x=='/') { *x=0; --x; }
  strncpy(buf+1,service,1400);
  buf[1400]=0;
}

int addreadwrite(char* service) {
  addservice(service);
  write(infd,buf,str_len(buf));
  return read(outfd,buf,1500);
}

/* return PID, 0 if error */
pid_t __readpid(char *service) {
  int len;
  buf[0]='p';
  len=addreadwrite(service);
  if (len<0) return 0;
  buf[len]=0;
  return atoi(buf);
}

/* return nonzero if error */
int respawn(char *service,int yesno) {
  int len;
  buf[0]=yesno?'R':'r';
  len=addreadwrite(service);
  return (len!=1 || buf[0]=='0');
}

/* return nonzero if error */
int setpid(char *service, pid_t pid) {
  char *tmp;
  int len;
  buf[0]='P';
  addservice(service);
  tmp=buf+str_len(buf)+1;
  tmp[fmt_ulong(tmp,pid)]=0;
  write(infd,buf,str_len(buf)+str_len(tmp)+2);
  len=read(outfd,buf,1500);
  return (len!=1 || buf[0]=='0');
}

/* return nonzero if error */
int check_remove(char *service) {
  int len;
  buf[0]='C';
  len=addreadwrite(service);
  return (len!=1 || buf[0]=='0');
}

/* return nonzero if error */
int startservice(char *service) {
  int len;
  buf[0]='s';
  len=addreadwrite(service);
  return (len!=1 || buf[0]=='0');
}

/* return uptime, 0 if error */
unsigned long uptime(char *service) {
  int len;
  buf[0]='u';
  len=addreadwrite(service);
  if (len<0) return 0;
  buf[len]=0;
  return atoi(buf);
}

void dumphistory() {
  char tmp[16384];
  int i,j;
  char first,last;
  first=1; last='x';
  write(infd,"h",1);
  for (;;) {
    int prev,done;
    j=read(outfd,tmp,sizeof(tmp));
    if (j<1) break;
    done=i=0;
    if (first) {
      if (tmp[0]=='0') {
	buffer_putsflush(buffer_2,"msvc: minit compiled without history support.\n");
	return;
      }
      i+=2;
    } else {
      if (!tmp[0] && last=='\n') break;
    }
    prev=i;
    for (; i<j; ++i)
      if (!tmp[i]) {
	tmp[i]=done?0:'\n';
	if (i<j && !tmp[i+1]) { done=1; --j; }
      }
    if (first)
      write(1,tmp+2,j-2);
    else
      write(1,tmp,j);
    if (done) break;
    last=tmp[j-1];
    first=0;
  }
}

void dumpdependencies(char* service) {
  char tmp[16384];
  int i,j;
  char first,last;
  buf[0]='d';
  addservice(service);
  write(infd,buf,str_len(buf));
  first=1; last='x';
  for (;;) {
    int prev,done;
    j=read(outfd,tmp,sizeof(tmp));
    if (j<1) break;
    done=i=0;
    if (first) {
      if (tmp[0]=='0') {
	buffer_puts(buffer_2,"msvc: ");
	buffer_puts(buffer_2,service);
	buffer_putsflush(buffer_2,": no such service.\n");
	return;
      }
      i+=2;
    } else {
      if (!tmp[0] && last=='\n') break;
    }
    prev=i;
    for (; i<j; ++i)
      if (!tmp[i]) {
	tmp[i]=done?0:'\n';
	if (i<j && !tmp[i+1]) { done=1; --j; }
      }
    if (first)
      write(1,tmp+2,j-2);
    else
      write(1,tmp,j);
    if (done) break;
    last=tmp[j-1];
    first=0;
  }
}

int main(int argc,char *argv[]) {
  if (argc<2) {
    buffer_putsflush(buffer_1,
	"usage: msvc -[uodpchaitkogC] service\n"
	"       msvc -Ppid service\n"
	" -u\tup; start service with respawn\n"
	" -o\tonce; start service without respawn\n"
	" -d\tdown; disable respawn, stop service\n"
	" -p\tpause; send SIGSTOP\n"
	" -c\tcontinue; send SIGCONT\n"
	" -h\thangup; send SIGHUP\n"
	" -a\talarm; send SIGALRM\n"
	" -i\tintr; send SIGINT\n"
	" -t\tterminate; send SIGTERM\n"
	" -k\tkill; send SIGKILL\n"
	" -g\tget; output just the PID\n"
	" -Ppid\tset PID of service (for pidfilehack)\n"
	" -D service\tprint services started as dependency\n"
	" -H\tprint last n respawned services\n"
	" -C\tClear; remove service form active list\n\n");
    return 0;
  }
  infd=open(MINITROOT "/in",O_WRONLY);
  outfd=open(MINITROOT "/out",O_RDONLY);
  if (infd>=0) {
    while (lockf(infd,F_LOCK,1)) {
      buffer_putsflush(buffer_2,"could not acquire lock!\n");
      sleep(1);
    }
    if (argc==2 && argv[1][1]!='H') {
      pid_t pid=__readpid(argv[1]);
      if (buf[0]!='0') {
	unsigned long len;
	buffer_puts(buffer_1,argv[1]);
	buffer_puts(buffer_1,": ");
	if (pid==0) buffer_puts(buffer_1,"down ");
	else if (pid==1) buffer_puts(buffer_1,"finished ");
	else {
	  buffer_puts(buffer_1,"up (pid ");
	  buffer_putulong(buffer_1,pid);
	  buffer_puts(buffer_1,") ");
	}
	len=uptime(argv[1]);
	buffer_putulong(buffer_1,len);
	buffer_putsflush(buffer_1," seconds\n");
	if (pid==0) return 2; else if (pid==1) return 3; else return 0;
      } else {
	buffer_puts(buffer_2,"msvc: ");
	buffer_puts(buffer_2,argv[1]);
	buffer_putsflush(buffer_2,": no such service.\n");
      }
      return 1;
    } else {
      int i;
      int ret=0;
      int sig=0;
      pid_t pid;
      if (argv[1][0]=='-') {
	switch (argv[1][1]) {
	case 'g':
	  for (i=2; i<argc; ++i) {
	    pid=__readpid(argv[i]);
	    if (pid<2) {
	      buffer_puts(buffer_2,"msvc: ");
	      buffer_puts(buffer_2,argv[i]);
	      buffer_putsflush(buffer_2,pid==1?": service terminated\n":": no such service\n");
	      ret=1;
	    }
	    buffer_putulong(buffer_1,pid);
	    buffer_putsflush(buffer_1,"\n");
	  }
	  break;
	case 'p': sig=SIGSTOP; goto dokill; break;
	case 'c': sig=SIGCONT; goto dokill; break;
	case 'h': sig=SIGHUP; goto dokill; break;
	case 'a': sig=SIGALRM; goto dokill; break;
	case 'i': sig=SIGINT; goto dokill; break;
	case 't': sig=SIGTERM; goto dokill; break;
	case 'k': sig=SIGKILL; goto dokill; break;
	case 'o':
	  for (i=2; i<argc; ++i)
	    if (startservice(argv[i]) || respawn(argv[i],0)) {
	      buffer_puts(buffer_2,"Could not start ");
	      buffer_puts(buffer_2,argv[i]);
	      buffer_putsflush(buffer_2,"\n");
	      ret=1;
	    }
	  break;
	case 'd':
	  for (i=2; i<argc; ++i) {
	    pid=__readpid(argv[i]);
	    if (pid==0) {
	      buffer_puts(buffer_2,"msvc: ");
	      buffer_puts(buffer_2,argv[i]);
	      buffer_putsflush(buffer_2,": no such service\n");
	      ret=1;
	    } else if (pid==1)
	      continue;
	    if (respawn(argv[i],0) || kill(pid,SIGTERM) || kill(pid,SIGCONT)) (void)0;
	  }
	  break;
	case 'u':
	  for (i=2; i<argc; ++i)
	    if (startservice(argv[i]) || respawn(argv[i],1)) {
	      buffer_puts(buffer_2,"Could not start ");
	      buffer_puts(buffer_2,argv[i]);
	      buffer_putsflush(buffer_2,"\n");
	      ret=1;
	    }
	  break;
	case 'C':
	  for (i=2; i<argc; ++i)
	    if (check_remove(argv[i])) {
	      buffer_puts(buffer_2,argv[i]);
	      buffer_putsflush(buffer_2," had terminated or was killed\n");
	      ret=1;
	    }
	  break;
	case 'P':
	  pid=atoi(argv[1]+2);
	  if (pid>1)
	    if (setpid(argv[2],pid)) {
	      buffer_puts(buffer_2,"Could not set pid of service ");
	      buffer_puts(buffer_2,argv[2]);
	      buffer_putsflush(buffer_2,"\n");
	      ret=1;
	    }
	  break;
	case 'H':
	  dumphistory();
	  break;
	case 'D':
	  dumpdependencies(argv[2]);
	  break;
	}
      }
      return ret;
dokill:
      for (i=2; i<argc; i++) {
	pid=__readpid(argv[i]);
	if (!pid) {
	  buffer_puts(buffer_2,"msvc: ");
	  buffer_puts(buffer_2,argv[i]);
	  buffer_putsflush(buffer_2,": no such service!\n");
	  ret=1;
	} else if (kill(pid,sig)) {
	  buffer_puts(buffer_2,"msvc: ");
	  buffer_puts(buffer_2,argv[i]);
	  buffer_puts(buffer_2,": could not send signal ");
	  buffer_putulong(buffer_2,sig);
	  buffer_puts(buffer_2," to PID ");
	  buffer_putulong(buffer_2,pid);
	  buffer_putsflush(buffer_2,"\n");
	  ret=1;
	}
      }
      return ret;
    }
  } else {
    buffer_putsflush(buffer_2,"minit: could not open " MINITROOT "/in or " MINITROOT "/out\n");
    return 1;
  }
}
