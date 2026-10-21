#ifndef FH_INSTALL_TRANSACTION_H
#define FH_INSTALL_TRANSACTION_H
#include <stdint.h>
#include <stdio.h>
enum {FH_INSTALL_ARTIFACTS=8,FH_INSTALL_NO_ARTIFACT=8,FH_INSTALL_TX_PATH=320};
/* IDs are recipe-owned: they are not source filenames or arbitrary SD paths.
 * Recipe and source keys must identify the complete conversion and inputs. */
typedef struct {
    uint32_t generation,serial,required,complete,partial,partial_bytes,partial_crc,cursor[2];
    uint32_t bytes[FH_INSTALL_ARTIFACTS],crc[FH_INSTALL_ARTIFACTS];
    unsigned char recipe[32],source[32];
} FhInstallGeneration;
typedef struct {
    FhInstallGeneration state;
    char root[240],error[112];
    FILE *output;
    uint32_t written,crc;
    unsigned slot,failed,checksum_dirty;
} FhInstallTransaction;
typedef int (*FhInstallValidate)(const char *path,void *context);
/* Zero initialize; all handles are owned by the caller's startup task. */
int fh_install_transaction_open(FhInstallTransaction*,const char *root,const unsigned char recipe[32],const unsigned char source[32],unsigned required);
/* Start a fresh generation after a damaged pending conversion. Never deletes
 * or reuses old generation files; the preceding active assets stay usable. */
int fh_install_transaction_restart(FhInstallTransaction*,const char *root,const unsigned char recipe[32],const unsigned char source[32],unsigned required);
void fh_install_transaction_close(FhInstallTransaction*);
int fh_install_artifact_begin(FhInstallTransaction*,unsigned id);
int fh_install_artifact_write(FhInstallTransaction*,const void*,uint32_t bytes);
/* Read owned staging bytes and restore append position; never GPU storage. */
int fh_install_artifact_read(FhInstallTransaction*,uint32_t offset,void*,uint32_t bytes);
/* Header finalization only: cannot overwrite a prefix covered by a checkpoint.
 * A stage using patches resumes from its preceding zero-length checkpoint. */
int fh_install_artifact_patch(FhInstallTransaction*,uint32_t offset,const void*,uint32_t bytes);
int fh_install_artifact_checkpoint(FhInstallTransaction*,uint32_t source_entry,uint32_t source_offset);
int fh_install_artifact_finish(FhInstallTransaction*,FhInstallValidate,void*);
int fh_install_transaction_publish(FhInstallTransaction*);
int fh_install_active(const char *root,const unsigned char recipe[32],const unsigned char source[32],unsigned required,FhInstallGeneration*);
int fh_install_artifact_path(const char *root,const FhInstallGeneration*,unsigned id,char out[FH_INSTALL_TX_PATH]);
#endif
