#define _GNU_SOURCE
#include <sys/types.h>
#include <open.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>

int write_to(const char* filename,const char* string) {
  int fd=open_write(filename);
  if (fd!=-1) {
    write(fd,string,strlen(string));
    fd=close(fd);
  }
  return fd;
}

void handle(const char* s) {
  size_t l=strlen(s);
  char* x=alloca(l+20);
  strcpy(x,s);
  strcpy(x+l,"/power/control");
  write_to(x,"auto");
}

void dwim(const char* name) {
  DIR* D;
  struct dirent* d;
  if (chdir(name)) return;
  if (!(D=opendir("."))) return;
  while ((d=readdir(D))) {
//    puts(d->d_name);
    handle(d->d_name);
  }
  closedir(D);
}

void pci() {
  dwim("/sys/bus/pci/devices");
}

void usb() {
  dwim("/sys/bus/usb/devices");
}

void wakeonlan(const char* interface) {
  struct ethtool_wolinfo {
   uint32_t cmd,supported,wolopts;
   char sopass[6];
  } wol;
  int fd=socket(PF_INET,SOCK_DGRAM,0);
  struct ifreq ifr;
  if (strlen(interface) > IFNAMSIZ)
    return;
  memset(&ifr,0,sizeof(ifr));
  strncpy(ifr.ifr_name,interface,sizeof(ifr.ifr_name));
  ifr.ifr_data=(void*)&wol;
  memset(&wol,0,sizeof(wol));
  wol.cmd=6;	// ETHTOOL_SWOL
  wol.wolopts=0;
  ioctl(fd,SIOCETHTOOL,&ifr);
  close(fd);
}

int main() {
  write_to("/sys/class/scsi_host/host0/link_power_management_policy","min_power");
  write_to("/sys/class/scsi_host/host1/link_power_management_policy","min_power");
  write_to("/sys/class/scsi_host/host2/link_power_management_policy","min_power");
  write_to("/sys/class/scsi_host/host3/link_power_management_policy","min_power");
  write_to("/sys/class/scsi_host/host4/link_power_management_policy","min_power");
  write_to("/sys/class/scsi_host/host5/link_power_management_policy","min_power");
  write_to("/sys/class/scsi_host/host6/link_power_management_policy","min_power");
  write_to("/sys/module/snd_hda_intel/parameters/power_save","1");
  write_to("/sys/bus/usb/devices/1-3/power/control","auto");
  write_to("/sys/bus/usb/devices/1-4/power/control","auto");
  write_to("/proc/sys/vm/dirty_writeback_centisecs","1500");
  pci();
  wakeonlan("eth0");
//  usb();
  return 0;
}
