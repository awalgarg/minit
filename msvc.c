#include <sys/fcntl.h>
#include <sys/file.h>
#include <signal.h>
#include <stdio.h>

static int infd,outfd;

static char buf[1500];

unsigned int fmt_ulong(register char *s,register unsigned long u)
{
  register unsigned int len; register unsigned long q;
  len = 1; q = u;
  while (q > 9) { ++len; q /= 10; }
  if (s) {
    s += len;
    do { *--s = '0' + (u % 10); u /= 10; } while(u); /* handles u == 0 */
  }
  return len;
}

/* return PID, 0 if error */
pid_t getpid(char *service) {
  int len;
  buf[0]='p';
  strncpy(buf+1,service,1400);
  write(infd,buf,strlen(buf));
  len=read(outfd,buf,1500);
  if (len<0) return 0;
  buf[len]=0;
  return atoi(buf);
}

/* return nonzero if error */
int respawn(char *service,int yesno) {
  int len;
  buf[0]=yesno?'R':'r';
  strncpy(buf+1,service,1400);
  write(infd,buf,strlen(buf));
  len=read(outfd,buf,1500);
  return (len!=1 || buf[0]=='0');
}

/* return nonzero if error */
int setpid(char *service, pid_t pid) {
  char *tmp;
  int len;
  buf[0]='P';
  tmp=buf+1+strncpy(buf+1,service,1400);
  tmp[fmt_ulong(tmp,pid)]=0;
  write(infd,buf,strlen(buf));
  len=read(outfd,buf,1500);
  return (len!=1 || buf[0]=='0');
}

/* return nonzero if error */
int startservice(char *service) {
  int len;
  buf[0]='s';
  strncpy(buf+1,service,1400);
  write(infd,buf,strlen(buf));
  len=read(outfd,buf,1500);
  return (len!=1 || buf[0]=='0');
}

/* return uptime, 0 if error */
unsigned long uptime(char *service) {
  int len;
  buf[0]='u';
  strncpy(buf+1,service,1400);
  write(infd,buf,strlen(buf));
  len=read(outfd,buf,1500);
  if (len<0) return 0;
  buf[len]=0;
  return atoi(buf);
}

main(int argc,char *argv[]) {
  int len;
  if (argc<2) {
    puts("usage: msvc -[uodpchaitko] service\n"
	 "       msvc -Ppid service\n"
	  " -u	up; start service with respawn\n"
	  " -o	once; start service without respawn\n"
	  " -d	down; disable respawn, stop service\n"
	  " -p	pause; send SIGSTOP\n"
	  " -c	continue; send SIGCONT\n"
	  " -h	hangup; send SIGHUP\n"
	  " -a	alarm; send SIGALRM\n"
	  " -i	intr; send SIGINT\n"
	  " -t	terminate; send SIGTERM\n"
	  " -k	kill; send SIGKILL\n");
  }
  infd=open("/etc/minit/in",O_WRONLY);
  outfd=open("/etc/minit/out",O_RDONLY);
  if (infd>=0) {
    while (lockf(infd,F_LOCK,1)) {
      puts("could not aquire lock!");
      sleep(1);
    }
    if (argc==2) {
      pid_t pid=getpid(argv[1]);
      if (buf[0]!='0') {
	unsigned long len;
	write(1,argv[1],strlen(argv[1]));
	write(1,": ",2);
	if (pid==0) write(1,"down ",5);
	else if (pid==1) write(1,"finished ",9);
	else {
	  write(1,"up (pid ",8);
	  snprintf(buf,30,"%d) ",pid);
	  write(1,buf,strlen(buf));
	}
	len=uptime(argv[1]);
	snprintf(buf,30,"%d seconds\n",len);
	write(1,buf,strlen(buf));
/*	puts(buf); */
      }
    } else {
      int i;
      int sig=0;
      pid_t pid;
      if (argv[1][0]=='-') {
	switch (argv[1][1]) {
	case 'p': sig=SIGSTOP; goto dokill; break;
	case 'c': sig=SIGCONT; goto dokill; break;
	case 'h': sig=SIGHUP; goto dokill; break;
	case 'a': sig=SIGALRM; goto dokill; break;
	case 'i': sig=SIGINT; goto dokill; break;
	case 't': sig=SIGTERM; goto dokill; break;
	case 'k': sig=SIGKILL; goto dokill; break;
	case 'o': /* TODO: start but don't restart */
	  if (startservice(argv[2]) || respawn(argv[2],0))
	    fprintf(stderr,"Could not start %s\n",argv[2]);
	  break;
	case 'd': /* TODO: down */
	  pid=getpid(argv[2]);
	  if (pid==0) {
	    puts("service not found");
	    return 1;
	  } else if (pid==1)
	    return 0;
	  if (respawn(argv[2],0) || kill(pid,SIGTERM));
	  break;
	case 'u': /* TODO: up */
	  if (startservice(argv[2]) || respawn(argv[2],1))
	    fprintf(stderr,"Could not start %s\n",argv[2]);
	  break;
	case 'P':
	  pid=atoi(argv[1]+2);
	  if (pid>1)
	    if (setpid(argv[2],pid))
	      fprintf(stderr,"Could not set pid of service %s\n",argv[2]);
	}
      }
      return 0;
dokill:
      for (i=2; i<=argc; i++) {
	pid=getpid(argv[2]);
	if (kill(pid,sig)) {
	  fprintf(stderr,"Could not send signal to PID %d\n",pid);
	}
      }
    }
  }
}

/*
  -u   Up.  If the service is not running, start it.  If the service stops,
       restart it.
  -d   Down.  If the service is running, send it a TERM signal and then a CONT
       signal.  After it stops, do not restart it.
  -o   Once.  If the service is not running, start it.  Do not restart it if it
       stops.
  -r   Tell supervise that the service is normally running; this affects status
       messages.
  -s   Tell supervise that the service is normally stopped; this affects status
       messages.
  -p   Pause.  Send the service a STOP signal.
  -c   Continue.  Send the service a CONT signal.
  -h   Hangup.  Send the service a HUP signal.
  -a   Alarm.  Send the service an ALRM signal.
  -i   Interrupt.  Send the service an INT signal.
  -t   Terminate.  Send the service a TERM signal.
  -k   Kill.  Send the service a KILL signal.
  -x   Exit.  supervise will quit as soon as the service is down.
*/
