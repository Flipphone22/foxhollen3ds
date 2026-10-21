#include <stdint.h>
#include <stddef.h>
#include <string.h>

void model_alias_write(void *destination, uint32_t value) {
    *(uint32_t *)destination = value;
}
void model_bulk_write(void *destination, const void *source, size_t bytes) {
    memcpy(destination, source, bytes);
}
void model_bulk_fill(void *destination, int value, size_t bytes) {
    memset(destination, value, bytes);
}
void model_byte_loop(unsigned char *destination, size_t bytes) {
    for(size_t i=0;i<bytes;i++) destination[i]=(unsigned char)(i*71+9);
}
void model_atomic_write(uint32_t *destination, uint32_t value) {
    __atomic_exchange_n(destination,value,__ATOMIC_SEQ_CST);
}
void model_unaligned_write(void *destination, uint32_t value) {
    struct __attribute__((packed)) Packed {uint32_t value;};
    ((struct Packed *)destination)->value=value;
}
