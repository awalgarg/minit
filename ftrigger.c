#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <stdlib.h>
#include <string.h>
#include <buffer.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <sys/signal.h>

struct trigger {
  const char* filename,* command;
  char* dironly,* fileonly;
  struct stat ss;
  int idd,idf;	/* inotify-deskriptor */
}* root;
int n;

void sighandler(int signum) {
  int status;
  (void)signum;
  wait(&status);
}

int v;

int main(int argc,char* argv[]) {
  int i,in;
  const char* command="make";
  struct stat ss;
  char buf[2048];
  struct inotify_event* ie=(struct inotify_event*)buf;
  struct pollfd p;
  static struct sigaction sa;

  sa.sa_flags=SA_RESTART|SA_NOCLDSTOP;
  sa.sa_handler=SIG_IGN;
  sigaction(SIGCHLD,&sa,0);
  in=inotify_init();
  root=(struct trigger*)alloca(sizeof(struct trigger)*argc);
  memset(root,0,sizeof(struct trigger)*argc);
  for (i=1; i<argc; ++i) {
    if (!strcmp(argv[i],"-v")) {
      v=1;
      continue;
    }
    if (argv[i][0]=='@') {
      command=argv[i]+1;
      ++i;
    }
    if (argv[i]) {
      char* c;
      root[n].filename=argv[i];
      root[n].command=command;
      root[n].dironly=alloca(strlen(root[n].filename)+3);
      c=strrchr(root[n].filename,'/');
      if (c) {
	size_t m=c-root[n].filename;
	strcpy(root[n].dironly,root[n].filename);
	root[n].fileonly=root[n].dironly+m+1;
	root[n].fileonly[-1]=0;
      } else {
	root[n].dironly[0]='.';
	root[n].dironly[1]=0;
	root[n].fileonly=root[n].dironly+2;
	strcpy(root[n].fileonly,root[n].filename);
      }
      ++n;
    }
  }

  for (i=0; i<n; ++i) {
    root[i].idf=-1;
    if (stat(root[i].filename,&root[i].ss)!=0)
      buffer_putmflush(buffer_2,"warning: could not stat file \"",root[i].filename,"\": ",strerror(errno),"\n");
    else
      root[i].idf=inotify_add_watch(in,root[i].filename,
				    IN_CLOSE_WRITE|IN_MOVE_SELF|IN_DELETE_SELF);
    root[i].idd=inotify_add_watch(in,root[i].dironly,
				  IN_MOVED_TO|IN_CREATE);
  }

  p.fd=in;
  p.events=POLLIN;

  for (;;) {
again:
    switch (poll(&p,1,1000)) {
    case -1:
      if (errno==EINTR) {
	int status;
	wait(&status);
	break;
      }
      return 1;
    case 0: continue;
    case 1: break;
    };
    read(in,buf,sizeof(buf));

#if 0
    buffer_puts(buffer_1,"got event for wd ");
    buffer_putulong(buffer_1,ie->wd);
    if (ie->len)
      buffer_putmflush(buffer_1," with filename \"",ie->name,"\"\n");
    else
      buffer_putsflush(buffer_1," with no associated filename\n");
    struct {
      unsigned int mask;
      const char* string;
    } strings[] = {
      { 0x00000001,"IN_ACCESS" },
      { 0x00000002,"IN_MODIFY" },
      { 0x00000004,"IN_ATTRIB" },
      { 0x00000008,"IN_CLOSE_WRITE" },
      { 0x00000010,"IN_CLOSE_NOWRITE" },
      { 0x00000020,"IN_OPEN" },
      { 0x00000040,"IN_MOVED_FROM" },
      { 0x00000080,"IN_MOVED_TO" },
      { 0x00000100,"IN_CREATE" },
      { 0x00000200,"IN_DELETE" },
      { 0x00000400,"IN_DELETE_SELF" },
      { 0x00000800,"IN_MOVE_SELF" },
      { 0x00002000,"IN_UNMOUNT" },
      { 0x00004000,"IN_Q_OVERFLOW" },
      { 0x00008000,"IN_IGNORED" },
    };

    for (i=0; i<(int)(sizeof(strings)/sizeof(strings[0])); ++i) {
      if (ie->mask&strings[i].mask)
	buffer_putm(buffer_1," ",strings[i].string);
    }
    buffer_putnlflush(buffer_1);

#endif

    for (i=0; i<n; ++i) {
      if (root[i].idf==ie->wd) {
	if (ie->mask & (IN_DELETE_SELF|IN_MOVE_SELF)) {
#if 0
	  buffer_putmflush(buffer_1,root[i].filename," was ",
			   ie->mask&IN_DELETE_SELF ? "deleted" : "renamed",
			   ", cancelling subscription\n");
#endif
	  inotify_rm_watch(in,ie->wd);
	  root[i].idf=-1;
	  goto again;
	}
	goto goodevent;
      } else if (root[i].idd==ie->wd) {
	if (!strcmp(ie->name,root[i].fileonly)) {
#if 0
	  buffer_putmflush(buffer_1,"the file we were interested in, ",
			    root[i].filename,", has been created. Adding subscription.\n");
#endif
	  if (root[i].idf!=-1)
	    inotify_rm_watch(in,root[i].idf);
	  root[i].idf=inotify_add_watch(in,root[i].filename,
				  IN_CLOSE_WRITE|IN_MOVE_SELF|IN_DELETE_SELF);
	  /* if the file was created, it will be empty now, wait for
	    * the IN_CLOSE_WRITE event. */
	  if (ie->mask & IN_CREATE) {
#if 0
	    buffer_putmflush(buffer_1,"file was created, so it will be empty now. Skipping until we get a close event.\n");
#endif
	    goto again;
	  }
	  goto goodevent;
	  /* if the file was moved, this is a genuine event we are
	    * interested in. fall through */
	} else goto again;
#if 0
	buffer_putmflush(buffer_1,"(directory event for \"",root[i].filename,"\")\n");
#endif
      }
    }
#if 0
    buffer_putmflush(buffer_1,"ignoring irrelevant event.\n");
#endif
    goto again;

goodevent:
#if 0
    buffer_putmflush(buffer_1,"got through to stat\n");
#endif
    for (i=0; i<n; ++i) {
      if (stat(root[i].filename,&ss)==0 && memcmp(&ss,&root[i].ss,sizeof(ss))) {
	memcpy(&root[i].ss,&ss,sizeof(ss));
	if (v)
	  buffer_putmflush(buffer_1,"file \"",root[i].filename,"\" changed, running command \"",root[i].command,"\"\n");
	if (vfork()==0) {
	  system(root[i].command);
	  exit(0);
	}
      }
    }
  }
  return 0;
}
