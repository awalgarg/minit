#include "buffer.h"

int buffer_putsflush(buffer* b,const char* x) {
  return buffer_putflush(b,x,strlen(x));
}
