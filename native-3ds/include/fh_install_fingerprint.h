#ifndef FH_INSTALL_FINGERPRINT_H
#define FH_INSTALL_FINGERPRINT_H
#include "fh_install_disc.h"
#include <mbedtls/sha256.h>
/* Preparation/resume only: never hash the ISO again on ordinary game startup.
 * Exclusive borrowed disc cursor until finish/cancel; the source stays open.
 * Each step reads at most 64 KiB. With the startup worker, one borrowed hash
 * overlaps the next read using a second 64 KiB normal-memory buffer. Join
 * precedes buffer reuse, digest completion and cancellation. No GPU access. */
typedef struct {
    FhInstallDisc *disc;
    unsigned char *buffer,*spare;
    mbedtls_sha256_context sha;
    unsigned char digest[32];
    uint32_t done,total,read,job_bytes;
    int job_status;
    long saved_position;
    unsigned active,complete,pending;
    char error[112];
} FhInstallFingerprint;
/* Zero initialize before first begin. begin cancels a previous active task. */
int fh_install_fingerprint_begin(FhInstallFingerprint*,FhInstallDisc*);
/* -1 failure, 0 more blocks, 1 complete; digest valid only with complete=1. */
int fh_install_fingerprint_step(FhInstallFingerprint*);
/* Restores borrowed cursor, frees scratch and invalidates any digest. */
void fh_install_fingerprint_cancel(FhInstallFingerprint*);
#endif
