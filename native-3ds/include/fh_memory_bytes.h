#ifndef FH_MEMORY_BYTES_H
#define FH_MEMORY_BYTES_H
#include <stddef.h>
#include <string.h>
/* CPU memory only; exact unsigned-byte ordering and requested bounds. */
int fh_memory_compare(const void *a,const void *b,size_t bytes);
void fh_memory_zero(void *out,size_t bytes);
void *fh_memory_copy(void *out,const void *in,size_t bytes);
/* Keep compiler-expanded small copies; bulk copies use bounded LDM/STM. */
#define fh_copy_bytes(d,s,n) (__builtin_constant_p(n) && (n)<=64 ? memcpy((d),(s),(n)) : fh_memory_copy((d),(s),(n)))
#endif
