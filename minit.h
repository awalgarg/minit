#define MINITROOT "/etc/minit"

#ifndef NOVARS
static struct process {
  char *name;
/*  char **argv; */
  pid_t pid;
  char respawn;
  char circular;
  time_t startedat;
  int __stdin,__stdout;
  int logservice;
} *root;

static int infd,outfd;
static int maxprocess=-1;
static int processalloc;
#endif
