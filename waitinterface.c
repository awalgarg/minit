#include <unistd.h>
#include <errmsg.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <string.h>
#include <fmt.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>

int main(int argc,char* argv[],char* envp[]) {
  unsigned int i;
  struct timespec req,rem;
  struct stat ss;
  char* sys;
  int found;
  errmsg_iam("waitinterface");
  if (argc<2)
    die(0,"usage: waitinterface wlan0 /usr/sbin/dhcpcd wlan0\n\twaits for wlan0 to show up and then executes the rest of the command line");
  sys=fmt_strm_alloca("/sys/class/net/",argv[1]);
  req.tv_sec=0; req.tv_nsec=100000000;
  for (i=0; i<100; ++i) {
    if ((found=lstat(sys,&ss))==0) break;
    nanosleep(&req,&rem);
  }
  if (found==-1)
    die(1,"interface not showing up");
  if (strstr(argv[0],"up")) {
    /* they want the interface to be up, too */
    int s=socket(PF_INET6,SOCK_DGRAM,IPPROTO_IP);
    struct ifreq ifr;
    if (s==-1) s=socket(PF_INET,SOCK_DGRAM,IPPROTO_IP);
    if (s==-1) diesys(1,"socket");
    strncpy(ifr.ifr_name,argv[1],sizeof(ifr.ifr_name));
    for (i=0; i<100; ++i) {
      if (ioctl(s,SIOCGIFFLAGS,&ifr)==0 && (ifr.ifr_flags&IFF_UP)) break;
      nanosleep(&req,&rem);
    }
    close(s);
    if (!(ifr.ifr_flags&IFF_UP))
      die(1,"interface not up");
  }
  if (argv[2]) {
    execve(argv[2],argv+2,envp);
    diesys(1,"execve");
  }
  return 0;
}
