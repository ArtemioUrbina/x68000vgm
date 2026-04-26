#include <stdint.h>

#ifdef VGZ_SUPPORT
uint8_t *decompress_vgz(uint8_t *compressed, uint32_t compressed_size, uint32_t *out_size);
#endif
