#ifndef FH_HARDWARE_H
#define FH_HARDWARE_H
/* Memory entitlement is selected before libc allocates heaps. APT confirms
 * the hardware model later, before any application worker or game allocation. */
extern unsigned fh_hardware_is_new,fh_hardware_large_memory;
void fh_hardware_init(void);
#endif
