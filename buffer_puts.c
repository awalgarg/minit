#include "buffer.h"

int buffer_puts(buffer* b,const char* x) {
  return buffer_put(b,x,strlen(x));
}
