#include <stdint.h>
#include <stdio.h>

uint32_t print_gd3_string(const char* label, uint8_t* vgm, uint32_t offset, uint32_t max_size) {    
    char chunk[128];
    int chunk_idx = 0;
    uint8_t end = 0;
    uint8_t label_printed = 0;

    if (offset >= max_size)
        return offset;

    while (!end && offset + 1 < max_size) {
        uint16_t character = vgm[offset] | (vgm[offset + 1] << 8);
        offset += 2; 
        
        if (character == 0)
            end = 1;
        else 
            if (character >= 0x20 && character <= 0x7E)
                chunk[chunk_idx++] = (char)character;

        if (chunk_idx == 127 || (end && chunk_idx > 0)) {
            chunk[chunk_idx] = '\0';
            
            if (!label_printed) {
                printf("%s%s", label, chunk);
                label_printed = 1;
            } else
                printf("%s", chunk);
            
            chunk_idx = 0;
        }
    }

    if (label_printed)
        printf("\n");
    
    return offset;
}

// Helper to silently skip over a UTF-16LE string (used to skip Japanese strings)
uint32_t skip_gd3_string(uint8_t* vgm, uint32_t offset, uint32_t max_size) {
    while (offset + 1 < max_size) {
        uint16_t character = vgm[offset] | (vgm[offset + 1] << 8);
        offset += 2;
        if (character == 0)
            return offset;
    }
    return offset;
}

void print_vgm_tags(uint8_t* vgm, uint32_t vgm_size) {
    if (vgm_size < 0x18)
        return;

    uint32_t gd3_rel_offset = vgm[0x14] | (vgm[0x15] << 8) | (vgm[0x16] << 16) | (vgm[0x17] << 24);
    
    if (gd3_rel_offset == 0) {
        return;
    }

    // Absolute offset is relative offset + the location of the offset pointer (0x14)
    uint32_t gd3_offset = gd3_rel_offset + 0x14;

    // Verify "Gd3 " magic number to ensure it's valid
    if (gd3_offset + 12 <= vgm_size && 
        vgm[gd3_offset] == 'G' && vgm[gd3_offset+1] == 'd' && 
        vgm[gd3_offset+2] == '3' && vgm[gd3_offset+3] == ' ') {
        
        // Strings start 12 bytes into the GD3 header
        uint32_t str_pos = gd3_offset + 12; 

        // GD3 Strings have English and Japanese.
        str_pos = print_gd3_string("Track  : ", vgm, str_pos, vgm_size); // 1. Track Name (En)
        str_pos = skip_gd3_string(vgm, str_pos, vgm_size);               // 2. Track Name (Jp)
        
        str_pos = print_gd3_string("Game   : ", vgm, str_pos, vgm_size); // 3. Game Name (En)
        str_pos = skip_gd3_string(vgm, str_pos, vgm_size);               // 4. Game Name (Jp)
        
        str_pos = print_gd3_string("System : ", vgm, str_pos, vgm_size); // 5. System (En)
        str_pos = skip_gd3_string(vgm, str_pos, vgm_size);               // 6. System (Jp)
        
        str_pos = print_gd3_string("Author : ", vgm, str_pos, vgm_size); // 7. Author (En)
    }
}
