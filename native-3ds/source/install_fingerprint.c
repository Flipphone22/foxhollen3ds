#include "fh_install_fingerprint.h"
#include "fh_install_worker.h"
#include <stdlib.h>
#include <string.h>
static int release(FhInstallFingerprint*f){
    int restored=1;
    /* The worker borrows both the context and buffer, including on I/O failure. */
    if(f->pending==1)fh_install_worker_join();
    f->pending=0;
    if(f->active){
        restored=f->disc&&f->disc->file&&!fseek(f->disc->file,f->saved_position,SEEK_SET);
        mbedtls_sha256_free(&f->sha);
    }
    free(f->buffer);free(f->spare);f->buffer=f->spare=NULL;f->active=0;f->disc=NULL;
    return restored;
}
void fh_install_fingerprint_cancel(FhInstallFingerprint*f){
    if(!f)return;
    if(!release(f))snprintf(f->error,sizeof(f->error),"Cannot restore source position after fingerprinting.");
    f->complete=0;memset(f->digest,0,sizeof(f->digest));
}
static int fail(FhInstallFingerprint*f,const char*why){
    fh_install_fingerprint_cancel(f);snprintf(f->error,sizeof(f->error),"%s",why);return -1;
}
int fh_install_fingerprint_begin(FhInstallFingerprint*f,FhInstallDisc*d){
    if(!f)return 0;
    fh_install_fingerprint_cancel(f);memset(f,0,sizeof(*f));
    if(!d||!d->file||!d->bytes){fail(f,"Open a validated source disc before fingerprinting.");return 0;}
    long position=ftell(d->file);
    if(position<0){fail(f,"Cannot locate the source disc cursor.");return 0;}
    f->disc=d;f->saved_position=position;f->total=d->bytes;f->active=1;
    mbedtls_sha256_init(&f->sha);
    f->buffer=malloc(FH_INSTALL_READ_CHUNK);
    if(!f->buffer||mbedtls_sha256_starts_ret(&f->sha,0)||fseek(d->file,0,SEEK_END)||
       ftell(d->file)!=(long)f->total||fseek(d->file,0,SEEK_SET)){
        fail(f,"Cannot initialize source disc fingerprint.");return 0;
    }
    FhInstallWorkerStats worker;fh_install_worker_stats(&worker);
    if(worker.active)f->spare=malloc(FH_INSTALL_READ_CHUNK);
    return 1;
}
static void hash_block(void*context){
    FhInstallFingerprint*f=context;
    f->job_status=mbedtls_sha256_update_ret(&f->sha,f->buffer,f->job_bytes);
}
int fh_install_fingerprint_step(FhInstallFingerprint*f){
    if(!f)return -1;
    if(f->complete)return 1;
    if(!f->active)return fail(f,"Source fingerprint task is not active.");
    uint32_t n=f->total-f->read;if(n>FH_INSTALL_READ_CHUNK)n=FH_INSTALL_READ_CHUNK;
    unsigned char*target=f->pending?f->spare:f->buffer;
    if(!f->disc->file||ftell(f->disc->file)!=(long)f->read||
       (n&&fread(target,1,n,f->disc->file)!=n))
        return fail(f,"Source disc changed or could not be read during fingerprinting.");
    f->read+=n;
    if(f->pending){
        if(f->pending==1)fh_install_worker_join();
        f->pending=0;
        if(f->job_status)return fail(f,"Source disc hash update failed.");
        f->done+=f->job_bytes;
        unsigned char*previous=f->buffer;f->buffer=f->spare;f->spare=previous;
    }
    if(n){
        f->job_bytes=n;
        if(f->spare){
            if(fh_install_worker_submit(hash_block,f))f->pending=1;
            else{hash_block(f);f->pending=2;} /* Synchronous completion, same cursor. */
            return 0;
        }
        hash_block(f);
        if(f->job_status)return fail(f,"Source disc hash update failed.");
        f->done+=n;
    }
    if(f->done<f->total)return 0;
    if(fseek(f->disc->file,0,SEEK_END)||ftell(f->disc->file)!=(long)f->total||
       mbedtls_sha256_finish_ret(&f->sha,f->digest))
        return fail(f,"Source disc fingerprint could not be completed.");
    if(!release(f))return fail(f,"Cannot restore source position after fingerprinting.");
    f->complete=1;return 1;
}
