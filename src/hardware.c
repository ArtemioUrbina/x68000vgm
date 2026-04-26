#include <stdint.h>
#include <string.h>
#include <dos.h>
#include "include/defines.h"

extern volatile uint32_t system_samples;

void YM2151_writeReg(uint8_t reg, uint8_t data) {
    volatile uint8_t *regPTR, *dataPTR;

    regPTR = (uint8_t*) YM2151_REG;
    dataPTR = (uint8_t*) YM2151_DATA;

    //wait BUSY
    while(*dataPTR & 0x80)
        ;
    *regPTR = reg;
    //wait BUSY
    while(*dataPTR & 0x80)
        ;
    *dataPTR = data;
}

void YM2151_mute() {
    uint8_t c;

    for(c = 0x60; c <= 0x7F; c++)
        YM2151_writeReg(c, 0x7F);

    for(c = 0; c < 8; c++)
        YM2151_writeReg(0x08, c); 
}

void YM2151_keyoff() {
    uint8_t c;

    for(c = 0; c < 8; c++)
        YM2151_writeReg(0x08, c); 
}


void MSM6258_writeReg(uint8_t *reg, uint8_t *data) {
    volatile uint8_t *regPTR, *dataPTR;

    regPTR = (uint8_t*) OKI6258_REG;
    dataPTR = (uint8_t*) OKI6258_DATA;

    *regPTR = *reg;
    *dataPTR = *data;
}

uint32_t selectMSM6258Freq(uint32_t target_freq){
    // Pre-calculated midpoints between the fixed hardware frequencies
    if (target_freq >= 13021) return ADPCM_SAMPLE_15_6KHZ;
    if (target_freq >= 9115)  return ADPCM_SAMPLE_10_4KHZ;
    if (target_freq >= 6511)  return ADPCM_SAMPLE_7_8KHZ;
    if (target_freq >= 4557)  return ADPCM_SAMPLE_5_2KHZ;
    
    return ADPCM_SAMPLE_3_9KHZ;
}

void swapHeader(VGMHEADER *header) {
    header->EOFoffset = swap_uint32_t(header->EOFoffset);
    header->version = swap_uint32_t(header->version);
    header->SN76489_clock = swap_uint32_t(header->SN76489_clock);
    header->YM2314_clock = swap_uint32_t(header->YM2314_clock);
    header->GD3_offset = swap_uint32_t(header->GD3_offset);
    header->total_samples = swap_uint32_t(header->total_samples);
    header->loop_offset = swap_uint32_t(header->loop_offset);
    header->loop_samples = swap_uint32_t(header->loop_samples);
    header->rate = swap_uint32_t(header->rate);
    header->SN76489_feedback = swap_uint16_t(header->SN76489_feedback);
    header->YM2612_clock = swap_uint32_t(header->YM2612_clock);
    header->YM2151_clock = swap_uint32_t(header->YM2151_clock);
    header->VGM_data_offset = swap_uint32_t(header->VGM_data_offset);
}

void readuint32_t(uint32_t *target, uint8_t *src, uint32_t *pos) {
    memcpy(target, src+*pos, sizeof(uint32_t));
    *target = swap_uint32_t(*target);
    *pos += sizeof(uint32_t);
}

void readuint16_t(uint16_t *target, uint8_t *src, uint32_t *pos) {
    memcpy(target, src+*pos, sizeof(uint16_t));
    *target = swap_uint16_t(*target);
    *pos += sizeof(uint16_t);
}

void interrupt timer_handler() {
    static int fraction = 0;

    system_samples += 22;
    fraction ++;
    if(fraction >= 20) {
        system_samples ++;
        fraction = 0;
    }
}

void flush_keyboard(void) {
    while(_dos_keysns())
        _dos_inkey();
}
