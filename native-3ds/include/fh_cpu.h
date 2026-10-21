#ifndef FH_CPU_H
#define FH_CPU_H
extern char fh_cpu_status[40];
void fh_platform_cpu_init(int is_new);
void fh_platform_cpu_poll(void);
void fh_platform_cpu_exit(void);
#endif
