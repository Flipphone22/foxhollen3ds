#ifndef FH_INSTALL_LIST_PACK_H
#define FH_INSTALL_LIST_PACK_H
/* Encode the same ordered, oriented immutable corner packets as triangles or
 * consecutive strips, whichever is smaller. Startup-only, no heap allocation.
 * Validates/measures before touching output; failure leaves it unchanged.
 * Bytes include original32-byte display-list padding and cannot exceed the
 * source's16-bit list-size limit. Caller must supply
 * distinct input/output storage and retain the original capacity limit. */
int fh_install_list_pack(void*output,unsigned capacity,const unsigned*indices,unsigned count,
                         const void*packets,unsigned vertices,unsigned stride,unsigned vat,unsigned*bytes);
#endif
