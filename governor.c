#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <libowfat/errmsg.h>

const char acstate[]="/proc/acpi/ac_adapter/AC/state";
const char sysacstate[]="/sys/class/power_supply/AC/online";
char governor[]="/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor";
const char powersave[]="powersave";
const char performance[]="performance";
const char ondemand[]="conservative";

int main(int argc,char* argv[]) {
  char buf[1024];
  int fd,r;
  const char* s;
  errmsg_iam("switch_governor");

  if (argc<1)
    s="governor";
  else if ((s=strrchr(argv[0],'/')))
    ++s;
  else
    s=argv[0];
  if (!strcmp(s,"slow")) {
    s=powersave; goto writeit;
  } else if (!strcmp(s,"fast")) {
    s=performance; goto writeit;
  } else if (!strcmp(s,"auto")) {
    s=ondemand; goto writeit;
  }

  /* first try /sys, if that fails attempt /proc */
  if ((fd=open(sysacstate,O_RDONLY))==-1) {
    if ((fd=open(acstate,O_RDONLY))==-1)
      diesys(1,"could not open ",acstate);
    if ((r=read(fd,buf,sizeof(buf)))<1)
      diesys(1,"read error");
    if (r>1000)
      die(1,"too much data in ",acstate);
    buf[r]=0;
    close(fd);
    if (strstr(buf,"off-line"))
      s=powersave;
    else if (strstr(buf,"on-line"))
      s=ondemand;
    else
      die(1,"ac adapter neither on-line nor off-line!?");
  } else {
    if ((r=read(fd,buf,sizeof(buf)))<1)
      diesys(1,"read error");
    if (r>2)
      die(1,"too much data in ",sysacstate);
    buf[r]=0;
    close(fd);
    if (buf[0]=='0')
      s=powersave;
    else if (buf[0]=='1')
      s=ondemand;
    else
      die(1,"ac adapter neither on-line nor off-line!?");
  }
writeit:
  {
    int i;
    char* x=strstr(governor,"cpu0");
    if (x) x+=3;
    for (i=0; i<32; ++i) {
      if ((fd=open(governor,O_WRONLY))==-1) {
	if (x && *x=='0')
	  diesys(1,"could not open ",governor);
	break;
      }
      r=strlen(s);
      if (write(fd,s,r)!=r)
	diesys(1,"write error");
      close(fd);
      if (x) ++(*x); else break;
    }
  }
  carp("switched cpufreq governor to ",s);
  return 0;
}
