#include <sys/fcntl.h>
#include <sys/file.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "fmt.h"
#include "buffer.h"

static int infd,outfd;

static char buf[1500];

/* return PID, 0 if error */
pid_t __readpid(char *service) {
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
  strncpy(buf+1,service,1400);
  tmp+=strlen(buf);
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
    buffer_putsflush(buffer_1,
         "usage: msvc -[uodpchaitko] service\n"
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
	  " -k	kill; send SIGKILL\n\n");
    return 0;
  }
  infd=open("/etc/minit/in",O_WRONLY);
  outfd=open("/etc/minit/out",O_RDONLY);
  if (infd>=0) {
    while (lockf(infd,F_LOCK,1)) {
      buffer_putsflush(buffer_2,"could not aquire lock!\n");
      sleep(1);
    }
    if (argc==2) {
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
	  if (startservice(argv[2]) || respawn(argv[2],0)) {
	    buffer_puts(buffer_2,"Could not start ");
	    buffer_puts(buffer_2,argv[2]);
	    buffer_putsflush(buffer_2,"\n");
	  }
	  break;
	case 'd': /* TODO: down */
	  pid=__readpid(argv[2]);
	  if (pid==0) {
	    buffer_putsflush(buffer_2,"service not found");
	    return 1;
	  } else if (pid==1)
	    return 0;
	  if (respawn(argv[2],0) || kill(pid,SIGTERM));
	  break;
	case 'u': /* TODO: up */
	  if (startservice(argv[2]) || respawn(argv[2],1)) {
	    buffer_puts(buffer_2,"Could not start ");
	    buffer_puts(buffer_2,argv[2]);
	    buffer_putsflush(buffer_2,"\n");
	  }
	  break;
	case 'P':
	  pid=atoi(argv[1]+2);
	  if (pid>1)
	    if (setpid(argv[2],pid)) {
	      buffer_puts(buffer_2,"Could not set pid of service ");
	      buffer_puts(buffer_2,argv[2]);
	      buffer_putsflush(buffer_2,"\n");
	    }
	}
      }
      return 0;
dokill:
      for (i=2; i<=argc; i++) {
	pid=__readpid(argv[2]);
	if (kill(pid,sig)) {
	  buffer_puts(buffer_2,"Could not send signal to PID ");
	  buffer_putulong(buffer_2,pid);
	  buffer_putsflush(buffer_2,"\n");
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
