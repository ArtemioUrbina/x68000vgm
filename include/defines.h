#include <stdint.h>
#include <iocs.h>

#define MAX_SAMPLES 50

#define GZIP_MAGIC_1  0x1F
#define GZIP_MAGIC_2  0x8B

// Learned from sample code from FedericoTech: https://github.com/FedericoTech/X68KTutorials/

#define swap_uint16_t(n) (((n << 8) & 0xff00) | ((n >> 8) & 0x00ff))
#define swap_uint32_t(n) (((n >> 24) & 0x000000ff) | ((n << 8) & 0x00ff0000) | ((n >> 8) & 0x0000ff00) | ((n << 24) & 0xff000000))

#define interrupt __attribute__ ((interrupt_handler))

#define ACCESS_DICTIONARY           0x80    //0b10000000 //1 not for users
#define ACCESS_NORMAL               0x00    //0b00000000 //0

#define SHARING_TOTAL               0x08    //0b00010000  //4 allow others to write and read
#define SHARING_WRITE_ONLY          0x0C    //0b00001100  //3 allow others to write only
#define SHARING_READ_ONLY           0x08    //0b00001000  //2 allow others to read only
#define SHARING_RESTRICTED          0x04    //0b00000100  //1 don't allow others anything
#define SHARING_COMPATIBILITY_MODE  0x00    //0b00000000  //0

#define MODE_RW                     0x02    //0b00000010  //2 open for write and read
#define MODE_W                      0x01    //0b00000001  //1 open only for writing
#define MODE_R                      0x00    //0b00000000  //0 open only for reading

#define OPENING_MODE(access, sharing, mode) (access | sharing | mode)

#define SEEK_MODE_BEGINNING         0
#define SEEK_MODE_CURRENT           1
#define SEEK_MODE_END               2


#define ADPCM_MODE_NO_WAIT          0x0000 //0b0000000000000000
#define ADPCM_MODE_WAIT             0x8000 //0b1000000000000000

#define ADPCM_SAMPLE_3_9KHZ         0x000  //0b0000000000000000
#define ADPCM_SAMPLE_5_2KHZ         0x100  //0b0000000100000000
#define ADPCM_SAMPLE_7_8KHZ         0x200  //0b0000001000000000
#define ADPCM_SAMPLE_10_4KHZ        0x300  //0b0000001100000000
#define ADPCM_SAMPLE_15_6KHZ        0x400  //0b0000010000000000

#define ADPCM_SPK_MUTE              0x00
#define ADPCM_SPK_LEFT              0x01
#define ADPCM_SPK_RIGHT             0x02
#define ADPCM_SPK_STEREO            0x03

#define ADPCM_MODE(mode, sample, speaker) (mode | sample | speaker)

#define ADPCM_CTRL_END              0
#define ADPCM_CTRL_SUSPEND          1
#define ADPCM_CTRL_RESUME           2
//_iocs_adpcmmod(int)

#define ADPCM_STATUS_IDLE           0
#define ADPCM_STATUS_OUT            2
#define ADPCM_STATUS_INP            3
#define ADPCM_STATUS_AOT            12
#define ADPCM_STATUS_AIN            14
#define ADPCM_STATUS_LOT            22
#define ADPCM_STATUS_LIN            24
// _iocs_adpcmsns()

#define YM2151_REG                  0xE90001
#define YM2151_DATA                 0xE90003

#define OKI6258_REG                 0xE92001
#define OKI6258_DATA                0xE92003

#define MFP_ADDR                    0xE88015

typedef struct {
    char        ident[4];
    uint32_t    EOFoffset;
    uint32_t    version;
    uint32_t    SN76489_clock;
    uint32_t    YM2314_clock;
    uint32_t    GD3_offset;
    uint32_t    total_samples;
    uint32_t    loop_offset;
    uint32_t    loop_samples;
    uint32_t    rate;
    uint16_t    SN76489_feedback;
    uint8_t     SN76489_shift;
    uint8_t     SN76489_flags;
    uint32_t    YM2612_clock;
    uint32_t    YM2151_clock;
    uint32_t    VGM_data_offset;
}VGMHEADER;

typedef struct {
    uint8_t *samples;
    uint32_t size;
} adpcm_samples;

// Number key definitions (group 0)
#define KEY_ESC     0x02
#define KEY_1       0x04
#define KEY_2       0x08
#define KEY_3       0x10
#define KEY_4       0x20
#define KEY_5       0x40
#define KEY_6       0x80

// Number key definitions (group 1)
#define KEY_7       0x01
#define KEY_8       0x02
#define KEY_9       0x04
#define KEY_0       0x08

#define KEY_CTRL    (_iocs_bitsns(14) & 0x02)
#define KEY_C       (_iocs_bitsns(5) & 0x10)

// Macro for handling unsopported DACs
#define HANDLE_DAC(SKIP_NAME, BYTES) {                  \
        pos += BYTES;                                   \
        static uint8_t printed_##SKIP_NAME = 0;         \
        if (!printed_##SKIP_NAME) {                     \
            _dos_print("Detected " #SKIP_NAME ", skipping DAC commands."); \
            printed_##SKIP_NAME = 1;                    \
        }                                               \
    }

