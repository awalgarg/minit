#define MINITROOT "/etc/minit"

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

