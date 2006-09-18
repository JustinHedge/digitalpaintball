#ifndef _MD5_H
#define _MD5_H

#ifdef __cplusplus
extern "C"
{
#endif

unsigned Com_MD5Checksum (void *buffer, int length);
unsigned Com_MD5ChecksumKey (void *buffer, int length, int key);
char *Com_MD5HashString (const void *buffer, int length, char *pMD5Out, size_t sizeMD5Out);

#ifdef __cplusplus
}
#endif

#endif

