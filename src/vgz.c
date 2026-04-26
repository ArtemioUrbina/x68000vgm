#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dos.h>

#ifdef VGZ_SUPPORT
#include "tinf/tinf.h"

void _exit(int status) {
    (void)status;
    _dos_exit();
}

// Returns a new _dos_malloc'd buffer with decompressed data, updates size.
// Frees the original compressed buffer on success.
// Returns NULL on failure (original buffer untouched).
uint8_t *decompress_vgz(uint8_t *compressed, uint32_t compressed_size, uint32_t *out_size) {
    uint8_t *decompressed = NULL;
    uint32_t dest_size = 0;

    // gzip ISIZE field: last 4 bytes of stream, little-endian
    dest_size = compressed[compressed_size-4]
              | (compressed[compressed_size-3] << 8)
              | (compressed[compressed_size-2] << 16)
              | (compressed[compressed_size-1] << 24);

    if(dest_size == 0) { 
        printf("\nVGZ decompressed size invalid: %lu.\n", dest_size);
        return NULL;
    }

    printf("from %lu to %lu bytes\n", compressed_size, dest_size);
    decompressed = (uint8_t*)_dos_malloc(dest_size);
    if(!decompressed) {
        printf("Not enough RAM to decompress VGZ (need %lu bytes).\n", dest_size);
        return NULL;
    }

    if(tinf_gzip_uncompress(decompressed, (unsigned int*)&dest_size,
                        compressed, compressed_size) != TINF_OK) {
        printf("VGZ decompression failed.\n");
        _dos_mfree(decompressed);
        return NULL;
    }

    *out_size = dest_size;
    return decompressed;
}

#endif
