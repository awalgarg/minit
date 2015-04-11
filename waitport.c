#include <unistd.h>
#include <errmsg.h>
#include <time.h>
#include <string.h>
#include <fmt.h>
#include <scan.h>
#include <ip4.h>
#include <ip6.h>
#include <stralloc.h>
#include <buffer.h>
#include <fcntl.h>
#include <ctype.h>

static int netstat(const char* addr,unsigned int wantedport) {
  /* see linux/Documentation/networking/proc_net_tcp.txt */
  int fd=-1;
  char rbuf[4096];	/* since these files can become arbitrarily large, we'll use a real buffer to read from them */
  const char* filenames[] = { "/proc/net/tcp6", "/proc/net/tcp" };
  buffer b;
  unsigned int fn;
  stralloc line;

  for (fn=0; fn<sizeof(filenames)/sizeof(filenames[0]); ++fn) {
    const char* filename=filenames[fn];

    fd=open(filename,O_RDONLY|O_CLOEXEC);
    if (fd==-1)
      continue;
    buffer_init(&b,read,fd,rbuf,sizeof(rbuf));	/* can't fail */
    for (;;) {
      int r;
      char* c;
      char* local;
      int v6;
      stralloc_zero(&line);
      if ((r=buffer_getline_sa(&b,&line))==-1) {
	close(fd);
	die(1,"read error from ",filename);
      }
      if (r==0) break;
      if (line.len < 1 || line.s[line.len-1]!='\n') {
parseerror:
	close(fd);
	die(1,"parse error in ",filename);
      }
      line.s[line.len-1]=0;	/* string is now null terminated */

      /* First token is something like "917:", skip */
      for (c=line.s; *c && *c!=':'; ++c) ;
      if (*c != ':') continue;	/* first line is boilerplate text and has no :-token, skip it */
      ++c;
      for (; *c==' '; ++c) ;
      /* Next token is something like "00000000:1770" or "00000000000000000000000000000000:0016" */
      local=c;
      for (; isxdigit(*c); ++c) ;
      if (c-local != 8 && c-local != 32)	/* we only support ipv4 and ipv6; this is neither */
	continue;
      v6=(c-local==32);
      if (*c!=':') goto parseerror;
      for (r=1; r<5; ++r) {
	if (!isxdigit(c[r])) goto parseerror;
      }
      if (c[5]!=' ') goto parseerror;
      c[5]=0;
      c+=6;
      /* Next token is the same thing, but we don't really need it, so
       * just skip till next whitespace */
      for (; *c && *c!=' '; ++c) ;
      if (*c!=' ') goto parseerror; ++c;
      /* Next is the state; if we are looking at tcp, 0A means LISTEN */
      if (filename[10]=='t' && c[0]=='0' && c[1]=='A') {
	/* TCP LISTEN */
	size_t n;
	union {
	  char ip[16];
	  uint32_t ints[4];
	} omgwtfbbq;
	char ipstring[FMT_IP6];
	unsigned short port;
	unsigned long temp;

	/* we can only be here if the hex string is 8 or 32 bytes long, see above */
	for (n=0; n<(unsigned int)v6*3+1; ++n) {
	  scan_xlongn(local+n*8,8,&temp);
	  omgwtfbbq.ints[n]=temp;
	}

	if (scan_xshort(local+(((unsigned int)v6*3+1)*8)+1,&port)!=4)	/* can't happen, we validated with isxdigit */
	  goto parseerror;

	ipstring[v6 ? fmt_ip6c(ipstring,omgwtfbbq.ip) : fmt_ip4(ipstring,omgwtfbbq.ip)]=0;

	if (!strcmp(ipstring,addr?addr : (v6?"::":"0.0.0.0")) && port==wantedport) {
	  close(fd);
	  return 1;
	}

      }
    }
    close(fd);
    fd=-1;
  }

  return 0;
}

int main(int argc,char* argv[],char* envp[]) {
  unsigned short port;

  unsigned int i;
  struct timespec req,rem;

  char* s=argv[1];
  char* t=strchr(s,'/');

  errmsg_iam("waitport");
  if (argc<2)
usage:
    die(0,"usage: waitport ::/111 some.rpcd\n\twaits for a service to bind to TCP port 111 on ::, then executes the rest of the command line");

  {
    if (t) {
      *t=0;
      if (scan_ushort(t+1,&port)==0) goto usage;
    } else {
      if (scan_ushort(s,&port)==0) goto usage;
      s=0;
    }
  }

  req.tv_sec=0; req.tv_nsec=100000000;
  for (i=0; i<1000; ++i) {
    if (netstat(s,port)) {
      if (argv[2]) {
	execve(argv[2],argv+2,envp);
	diesys(1,"execve");
      }
      return 0;
    }
    nanosleep(&req,&rem);
  }
  die(1,"service on port not showing up");
}
