#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <iocs.h>
#include <dos.h>
#include "include/defines.h"
#include "include/hardware.h"
#include "include/gd3.h"
#include "include/vgz.h"

// Learned from sample code from FedericoTech: https://github.com/FedericoTech/X68KTutorials/

volatile uint32_t system_samples = 0;

int main(int argc, char *argv[])
{
    char            c = 0;
    uint8_t         fm = 1, adpcm = 1, *vgm = NULL, numsamples = 0;
    uint8_t         repeat = 0, end = 0;
    uint32_t        pos = 0, vgmSize = 0, start = 0;
    uint32_t        currentTicks = 0, adpcmfreq = ADPCM_SAMPLE_3_9KHZ;
    int32_t         old_sp = 0;
    int16_t         file = 0, keypress = 0;
    VGMHEADER       header;
    adpcm_samples   samples[MAX_SAMPLES];
    void            *old_opm_vector = NULL;
    uint8_t         mfp_imrb_old = 0;
    uint8_t         mfp_ierb_old = 0;
    volatile uint8_t *MFP_IERB = (uint8_t *)0xE88009; // Interrupt Enable
    volatile uint8_t *MFP_IMRB = (uint8_t *)0xE88015; // Interrupt Mask

    printf("Quick Hack VGM player 1.01 for X68000 by Artemio Urbina 2026\n");

    if(argc < 2) {
        printf("Please define a VGM file\n");
        _dos_exit();
    }

    if(argc >= 3) {
        for(int i = 2; i < argc; i++) {
            if(argv[i][0] == 'r') {
                printf("Repeat On. ESC exits.\n");
                repeat = 1;
            }
        }
    }

    file = _dos_open(argv[1], OPENING_MODE(
            ACCESS_NORMAL,
            SHARING_COMPATIBILITY_MODE,
            MODE_R
        ));
    if(file < 0) {
        printf("Could not load file %s\n", argv[1]);
        _dos_exit();
    }

    memset(&header, 0, sizeof(VGMHEADER));
    memset(samples, 0, sizeof(adpcm_samples)*MAX_SAMPLES);

    printf("Loading %s\n", argv[1]);

    vgmSize = _dos_seek(file, 0L, SEEK_MODE_END);
    _dos_seek(file, 0L, SEEK_MODE_BEGINNING);

    vgm = (uint8_t*)_dos_malloc(sizeof(uint8_t)*vgmSize);
    if(!vgm) {
        _dos_close(file);
        printf("Not enough RAM for VGM file %s (needed %lu bytes)\n",
                argv[1], vgmSize);
        _dos_exit();
    }

    if(_dos_read(file, (char*)vgm, sizeof(uint8_t)*vgmSize) != vgmSize) {
        _dos_close(file);
        _dos_mfree(vgm);
        printf("Could not load VGM file %s into RAM\n", argv[1]);
        _dos_exit();
    }

    if(_dos_close(file) < 0){
        _dos_mfree(vgm);
        printf("Can't close the VGM file\n");
        _dos_exit();
    }

    if(vgm[0] == GZIP_MAGIC_1 && vgm[1] == GZIP_MAGIC_2) {
#ifdef VGZ_SUPPORT
        uint8_t     *uncompressed = NULL;
        uint32_t    decompressed_size = 0;

        printf("Decompressing VGM ");
        uncompressed = decompress_vgz(vgm, vgmSize, &decompressed_size);
        _dos_mfree(vgm);
        vgm = uncompressed;
        if(!uncompressed) {
            _dos_exit();
        }
        vgmSize = decompressed_size;
#else
        printf("Detected VGZ (compressed VGM), please compile with tinf.\n");
        _dos_exit();
#endif
    }

    memcpy(&header, vgm, sizeof(VGMHEADER));

    if(strncmp("Vgm", header.ident, 3) != 0) {
        printf("Not a VGM file, invalid header\n");
        _dos_mfree(vgm);
        _dos_exit();
    }

    swapHeader(&header);

    if(header.VGM_data_offset == 0x0000000C || header.VGM_data_offset == 0) 
        pos = 0x40;
    else
        pos = 0x34 + header.VGM_data_offset;
    start = pos;

    printf("VGM version %lX\n", header.version);

    print_vgm_tags(vgm, vgmSize);

    old_sp = _dos_super(0);

    YM2151_mute();
    if(_iocs_adpcmsns() != ADPCM_STATUS_IDLE)
        _iocs_adpcmmod(ADPCM_CTRL_END);

    // OPM  X68000 Vector $43 (YM2151 Interrupt)
    old_opm_vector = (void *)_iocs_b_intvcs(0x43, (void *)opm_timer_handler);

    // Unmask FM interrupt in the X68000 MFP (Bit 3 of IERA)
    mfp_ierb_old = *MFP_IERB;
    mfp_imrb_old = *MFP_IMRB;
    *MFP_IERB = mfp_ierb_old | 0x08; 
    *MFP_IMRB = mfp_imrb_old | 0x08;

    // Timer A for 1008us (Count = 63 / 0x3C1 inverse)
    YM2151_writeReg(0x10, 0xF0); // Timer A MSB
    YM2151_writeReg(0x11, 0x01); // Timer A LSB
    YM2151_writeReg(0x14, 0x15);
    
    do {
        uint8_t command = 0;
        static uint8_t prev_grp0 = 0;

        if(currentTicks > system_samples || keypress++ >= 500) {
            uint8_t grp0 = _iocs_bitsns(0);
            uint8_t newly_pressed = grp0 & ~prev_grp0;  // only bits that just went high

            prev_grp0 = grp0;

            if(newly_pressed & KEY_ESC || (KEY_CTRL && KEY_C))
                end = 1;

            if(newly_pressed & KEY_1) {
                fm = !fm;
                if(!fm)
                    YM2151_keyoff();
            }
            if(newly_pressed & KEY_2) {
                adpcm = !adpcm;
                if(!adpcm)
                    _iocs_adpcmmod(ADPCM_CTRL_END);
            }
            keypress = 0;
        }

        while(currentTicks > system_samples && !end) {
            // spin
        }

        if(pos < vgmSize) {
            c = vgm[pos++];
            command = (uint8_t)c;
        }
        else
            command = 0x66;

        if(command >= 0x70 && command <= 0x7F)
            command = 0x70;

        if(end)
            command = 0x66;

        switch(command) {
            case 0x54: { // YM2151 command
                uint8_t reg, data;

                reg = vgm[pos++];
                data = vgm[pos++];
                if(fm) {
                    // Block VGM from overwriting OPM timer registers
                    if (!(reg >= 0x10 && reg <= 0x14))
                        YM2151_writeReg(reg, data);
                }
            }
            break;
            case 0x61: { // Wait nn nn
                uint16_t ticks;

                readuint16_t(&ticks, vgm, &pos);
                currentTicks += ticks;
            }
            break;
            case 0x62:
                currentTicks += 735;
            break;
            case 0x63:
                currentTicks += 882;
            break;
            case 0x66:
                if(!repeat)
                    end = 1;
                else {
                    if(header.loop_offset != 0)
                        pos = header.loop_offset + 0x1C;
                    else
                        pos = start;
                }
            break;
            case 0x67: { // data block
                if(vgm[pos++] == 0x66) {
                    uint8_t type;
                    uint32_t datasize;

                    type = vgm[pos++];
                    readuint32_t(&datasize, vgm, &pos);

                    if(type == 0x04) {  // MSM6258
                        if(datasize > 65280)
                            printf("Sample %d breaks DMA limit (%lu bytes)\n",
                                numsamples, datasize);
                        samples[numsamples].samples = &(vgm[pos]);
                        samples[numsamples].size = datasize;

                        numsamples ++;
                        if(numsamples >= MAX_SAMPLES) {
                            printf("Max Samples reached. Increase MAX_SAMPLES and recompile\n");
                            end = 1;
                        }
                        else
                            _dos_print("*");
                    }
                    else {
                        static uint8_t supportMsg = 0;
                        if(!supportMsg) {
                            printf("Got data block 0x%X. Ignoring\n", type);
                            supportMsg = 1;
                        }
                    }

                    pos += datasize;
                    start = pos;
                }
                else {
                    printf("Invalid byte in 0x67 command: 0x%02X Aborting.", vgm[pos - 1]);
                    end = 1;
                }
            }
            break;
            case 0x70:  // ticks
                currentTicks += ((uint8_t)0x0f & (uint8_t)c) + 1;
            break;
            case 0xB7: { // MSM6258 Reg write, 
                uint8_t reg, data;

                reg = vgm[pos++];
                data = vgm[pos++];
                if(adpcm)
                    MSM6258_writeReg(&reg, &data);
            }
            break;
            case 0x90: // Setup stream control
                pos += 4;
            break;
            case 0x91: { // Set Stream Data
                uint8_t id, dataid;

                id = vgm[pos++];
                dataid = vgm[pos++];
                pos += 2; // step & stepsize

                // First DAC and ADPCM stream (4)
                if(id != 0 || dataid != 4) {
                    printf("Non ADPCM(4) stream detected 0x%X/0x%X\n", id, dataid);
                }
            }
            break;
            case 0x92: { // Set Stream freq
                uint32_t freq;

                pos++;  // id
                readuint32_t(&freq, vgm, &pos);
                if(adpcm) 
                    adpcmfreq = selectMSM6258Freq(freq);

            }
            break;
            case 0x93: { // Start Stream
                uint8_t id, lenmode;
                uint32_t offset, length;

                id = vgm[pos++];
                readuint32_t(&offset, vgm, &pos);
                lenmode = vgm[pos++];
                readuint32_t(&length, vgm, &pos);
                printf("Start Stream %X offset 0x%lX mode %X len 0x%lX\n", 
                        id, offset, lenmode, length);
            }
            break;
            case 0x94: { // Stop Stream
                pos++; // id
                if(adpcm)
                    _iocs_adpcmmod(ADPCM_CTRL_END);
            }
            break;
            case 0x95: { // Start Stream (fast call)
                uint16_t bid; // from command 0x91

                pos++;   // id
                readuint16_t(&bid, vgm, &pos);
                pos++;  // flags

                if(bid < numsamples) {
                    if(adpcm) {
                        if(_iocs_adpcmsns() != ADPCM_STATUS_IDLE)
                            _iocs_adpcmmod(ADPCM_CTRL_END);

                        _iocs_adpcmout(samples[bid].samples,
                                        ADPCM_MODE(
                                            ADPCM_MODE_NO_WAIT,
                                            adpcmfreq,
                                            ADPCM_SPK_STEREO
                                        ),
                                        samples[bid].size
                                    );
                    }
                }
                else
                    printf("Received invalid sample id %X\n", bid);
            }
            break;
            case 0xA0:  // Command for AY8910, Skip!
                HANDLE_DAC(AY8910, 2);
            break;
            case 0xB8:  // Command for MSM6295, Skip!
                HANDLE_DAC(MSM6295, 2);
            break;
            case 0xBA:  // Command for K053260, Skip!
                HANDLE_DAC(K053260, 2);
            break;
            case 0xBF:  // Command for GA20, Skip!
                HANDLE_DAC(GA20, 2);
            break;
            case 0xC0:  // Command for SegaPCM, Skip!
                HANDLE_DAC(SegaPCM, 3);
            break;
            case 0xD3:  // Command for K054539, Skip!
                HANDLE_DAC(K054539, 3);
            break;
            case 0xD4:  // Command for C140, Skip!
                HANDLE_DAC(C140, 3);
            break;
            default: {
                static uint8_t unkComMsg = 0;
                if(!unkComMsg) {
                    printf("Unknown command 0x%X at 0x%lX\n", command, pos - 1);
                    unkComMsg = 1;
                }
            }
            break;

        }
    } while(pos < vgmSize && !end);

    YM2151_writeReg(0x14, 0x00);
    *MFP_IMRB = mfp_imrb_old;
    *MFP_IERB = mfp_ierb_old;
    _iocs_b_intvcs(0x43, old_opm_vector);

    _iocs_adpcmmod(ADPCM_CTRL_END);
    YM2151_mute();

    _dos_mfree(vgm);
    vgm = NULL;

    flush_keyboard();
    printf("\n");

    _dos_super(old_sp); 
    _dos_exit();
}

