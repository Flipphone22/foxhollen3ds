#ifndef FH_DIAGNOSTIC_BUFFER_H
#define FH_DIAGNOSTIC_BUFFER_H
#include <stdio.h>
/* Six diagnostic streams, at most 384 KiB normal RAM. Short console runs
 * retain telemetry until normal close or panic, avoiding repeated SD file
 * growth while assets stream. Full buffers still use the owned I/O queue;
 * no records are dropped. This is not a save-file buffering policy. */
static inline int fh_diagnostic_buffer(FILE *file) {
    return file?setvbuf(file,NULL,_IOFBF,65536):0;
}
#endif
