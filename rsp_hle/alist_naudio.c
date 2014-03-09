/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-rsp-hle - alist_naudio.c                                  *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2014 Bobby Smiles                                       *
 *   Copyright (C) 2009 Richard Goedeken                                   *
 *   Copyright (C) 2002 Hacktarux                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef _MSC_VER
#include <stdbool.h>
#else
#include "mystdbool.h"
#endif
#include <stdint.h>

#include "alist_internal.h"
#include "hle_external.h"
#include "hle_internal.h"
#include "memory.h"

void MP3(struct hle_t* hle, uint32_t w1, uint32_t w2);

enum { NAUDIO_COUNT = 0x170 }; /* ie 184 samples */
enum {
    NAUDIO_MAIN      = 0x4f0,
    NAUDIO_MAIN2     = 0x660,
    NAUDIO_DRY_LEFT  = 0x9d0,
    NAUDIO_DRY_RIGHT = 0xb40,
    NAUDIO_WET_LEFT  = 0xcb0,
    NAUDIO_WET_RIGHT = 0xe20
};


/* audio commands definition */
static void UNKNOWN(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
    uint8_t acmd = (w1 >> 24);

    HleWarnMessage(hle->user_defined,
                   "Unknown audio command %d: %08x %08x",
                   acmd, w1, w2);
}


static void SPNOOP(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
}

static void NAUDIO_0000(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
    /* ??? */
    UNKNOWN(hle, w1, w2);
}

static void NAUDIO_02B0(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
    uint32_t rate = (hle->alist_naudio.rate[1] & 0xffff0000) | (w2 & 0xffff);
    hle->alist_naudio.rate[1] = rate;
}

static void NAUDIO_14(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
    if (hle->alist_naudio.table[0] == 0 && hle->alist_naudio.table[1] == 0) {

        uint8_t  flags       = (w1 >> 16);
        uint16_t gain        = w1;
        uint8_t  select_main = (w2 >> 24);
        uint32_t address     = (w2 & 0xffffff);

        uint16_t dmem = (select_main == 0) ? NAUDIO_MAIN : NAUDIO_MAIN2;

        alist_polef(
                hle,
                flags & A_INIT,
                dmem,
                dmem,
                NAUDIO_COUNT,
                gain,
                hle->alist_naudio.table,
                address);
    }
    else
        HleWarnMessage(hle->user_defined,
                       "NAUDIO_14: non null codebook[0-3] case not implemented.");
}

static void SETVOL(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
    uint8_t flags = (w1 >> 16);

    if (flags & 0x4) {
        if (flags & 0x2) {
            hle->alist_naudio.vol[0] = w1;
            hle->alist_naudio.dry    = (w2 >> 16);
            hle->alist_naudio.wet    = w2;
        }
        else {
            hle->alist_naudio.target[1] = w1;
            hle->alist_naudio.rate[1]   = w2;
        }
    }
    else {
        hle->alist_naudio.target[0] = w1;
        hle->alist_naudio.rate[0]   = w2;
    }
}

static void ENVMIXER(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
    uint8_t  flags   = (w1 >> 16);
    uint32_t address = (w2 & 0xffffff);

    hle->alist_naudio.vol[1] = w1;

    alist_envmix_lin(
            hle,
            flags & 0x1,
            NAUDIO_DRY_LEFT,
            NAUDIO_DRY_RIGHT,
            NAUDIO_WET_LEFT,
            NAUDIO_WET_RIGHT,
            NAUDIO_MAIN,
            NAUDIO_COUNT,
            hle->alist_naudio.dry,
            hle->alist_naudio.wet,
            hle->alist_naudio.vol,
            hle->alist_naudio.target,
            hle->alist_naudio.rate,
            address);
}

static void CLEARBUFF(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
    uint16_t dmem  = w1 + NAUDIO_MAIN;
    uint16_t count = w2;

    alist_clear(hle, dmem, count);
}

static void MIXER(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
    int16_t  gain  = w1;
    uint16_t dmemi = (w2 >> 16) + NAUDIO_MAIN;
    uint16_t dmemo = w2 + NAUDIO_MAIN;

    alist_mix(hle, dmemo, dmemi, NAUDIO_COUNT, gain);
}

static void LOADBUFF(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
    uint16_t count   = (w1 >> 12) & 0xfff;
    uint16_t dmem    = (w1 & 0xfff) + NAUDIO_MAIN;
    uint32_t address = (w2 & 0xffffff);

    alist_load(hle, dmem, address, count);
}

static void SAVEBUFF(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
    uint16_t count   = (w1 >> 12) & 0xfff;
    uint16_t dmem    = (w1 & 0xfff) + NAUDIO_MAIN;
    uint32_t address = (w2 & 0xffffff);

    alist_save(hle, dmem, address, count);
}

static void LOADADPCM(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
    uint16_t count   = w1;
    uint32_t address = (w2 & 0xffffff);

    dram_load_u16(hle, (uint16_t*)hle->alist_naudio.table, address, count >> 1);
}

static void DMEMMOVE(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
    uint16_t dmemi = w1 + NAUDIO_MAIN;
    uint16_t dmemo = (w2 >> 16) + NAUDIO_MAIN;
    uint16_t count = w2;

    alist_move(hle, dmemo, dmemi, (count + 3) & ~3);
}

static void SETLOOP(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
    hle->alist_naudio.loop = (w2 & 0xffffff);
}

static void ADPCM(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
    uint32_t address = (w1 & 0xffffff);
    uint8_t  flags   = (w2 >> 28);
    uint16_t count   = (w2 >> 16) & 0xfff;
    uint16_t dmemi   = ((w2 >> 12) & 0xf) + NAUDIO_MAIN;
    uint16_t dmemo   = (w2 & 0xfff) + NAUDIO_MAIN;

    alist_adpcm(
            hle,
            flags & 0x1,
            flags & 0x2,
            false,          /* unsuported by this ucode */
            dmemo,
            dmemi,
            (count + 0x1f) & ~0x1f,
            hle->alist_naudio.table,
            hle->alist_naudio.loop,
            address);
}

static void RESAMPLE(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
    uint32_t address = (w1 & 0xffffff);
    uint8_t  flags   = (w2 >> 30);
    uint16_t pitch   = (w2 >> 14);
    uint16_t dmemi   = ((w2 >> 2) & 0xfff) + NAUDIO_MAIN;
    uint16_t dmemo   = (w2 & 0x3) ? NAUDIO_MAIN2 : NAUDIO_MAIN;

    alist_resample(
            hle,
            flags & 0x1,
            false,          /* TODO: check which ABI supports it */
            dmemo,
            dmemi,
            NAUDIO_COUNT,
            pitch << 1,
            address);
}

static void INTERLEAVE(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
    alist_interleave(hle, NAUDIO_MAIN, NAUDIO_DRY_LEFT, NAUDIO_DRY_RIGHT, NAUDIO_COUNT);
}

static void MP3ADDY(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
}

/* global functions */
void alist_process_naudio(struct hle_t* hle)
{
    static const acmd_callback_t ABI[0x10] = {
        SPNOOP,         ADPCM,          CLEARBUFF,      ENVMIXER,
        LOADBUFF,       RESAMPLE,       SAVEBUFF,       NAUDIO_0000,
        NAUDIO_0000,    SETVOL,         DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     NAUDIO_02B0,    SETLOOP
    };

    alist_process(hle, ABI, 0x10);
}

void alist_process_naudio_bk(struct hle_t* hle)
{
    /* TODO: see what differs from alist_process_naudio */
    static const acmd_callback_t ABI[0x10] = {
        SPNOOP,         ADPCM,          CLEARBUFF,      ENVMIXER,
        LOADBUFF,       RESAMPLE,       SAVEBUFF,       NAUDIO_0000,
        NAUDIO_0000,    SETVOL,         DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     NAUDIO_02B0,    SETLOOP
    };

    alist_process(hle, ABI, 0x10);
}

void alist_process_naudio_dk(struct hle_t* hle)
{
    /* TODO: see what differs from alist_process_naudio */
    static const acmd_callback_t ABI[0x10] = {
        SPNOOP,         ADPCM,          CLEARBUFF,      ENVMIXER,
        LOADBUFF,       RESAMPLE,       SAVEBUFF,       MIXER,
        MIXER,          SETVOL,         DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     NAUDIO_02B0,    SETLOOP
    };

    alist_process(hle, ABI, 0x10);
}

void alist_process_naudio_mp3(struct hle_t* hle)
{
    static const acmd_callback_t ABI[0x10] = {
        UNKNOWN,        ADPCM,          CLEARBUFF,      ENVMIXER,
        LOADBUFF,       RESAMPLE,       SAVEBUFF,       MP3,
        MP3ADDY,        SETVOL,         DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     NAUDIO_14,      SETLOOP
    };

    alist_process(hle, ABI, 0x10);
}

void alist_process_naudio_cbfd(struct hle_t* hle)
{
    /* TODO: see what differs from alist_process_naudio_mp3 */
    static const acmd_callback_t ABI[0x10] = {
        UNKNOWN,        ADPCM,          CLEARBUFF,      ENVMIXER,
        LOADBUFF,       RESAMPLE,       SAVEBUFF,       MP3,
        MP3ADDY,        SETVOL,         DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     NAUDIO_14,      SETLOOP
    };

    alist_process(hle, ABI, 0x10);
}
