#include <stdint.h>

void YM2151_writeReg(uint8_t reg, uint8_t data);
void YM2151_keyoff();
void YM2151_mute();

void MSM6258_writeReg(uint8_t *reg, uint8_t *data);
uint32_t selectMSM6258Freq(uint32_t target_freq);

void readuint32_t(uint32_t *target, uint8_t *src, uint32_t *pos);
void readuint16_t(uint16_t *target, uint8_t *src, uint32_t *pos);
void swapHeader(VGMHEADER *header);

void interrupt opm_timer_handler();
void flush_keyboard(void);