#ifdef __cplusplus
extern "C"
{
#endif

unsigned Com_BlockChecksum (void *buffer, int length);
unsigned Com_BlockChecksumKey (void *buffer, int length, int key);

#ifdef __cplusplus
}
#endif