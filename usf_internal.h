#ifndef _USF_INTERNAL_H_
#define _USF_INTERNAL_H_

#include "cpu.h"

struct usf_state_helper
{
    size_t offset_to_structure;
};

#ifndef RCPREG_DEFINED
#define RCPREG_DEFINED
typedef uint32_t RCPREG;
#endif

#include <stdio.h>

// rsp_hle/alist_audio.c
enum { DMEM_BASE = 0x5c0 };
enum { N_SEGMENTS = 16 };

// rsp_hle/alist_naudio.c
enum { NAUDIO_COUNT = 0x170 }; /* ie 184 samples */
enum {
    NAUDIO_MAIN      = 0x4f0,
    NAUDIO_MAIN2     = 0x660,
    NAUDIO_DRY_LEFT  = 0x9d0,
    NAUDIO_DRY_RIGHT = 0xb40,
    NAUDIO_WET_LEFT  = 0xcb0,
    NAUDIO_WET_RIGHT = 0xe20
};

struct usf_state
{
    // RSP vector registers, need to be aligned to 16 bytes
    // when SSE2 or SSSE3 is enabled, or for any hope of
    // auto vectorization

    // usf_clear takes care of aligning the structure within
    // the memory block passed into it, treating the pointer
    // as usf_state_helper, and storing an offset from the
    // pointer to the actual usf_state structure. The size
    // which is indicated for allocation accounts for this
    // with two pages of padding.

    short VR[32][8];
    short VACC[3][8];
    
    // RSP virtual registers, also needs alignment
    int SR[32];
    
    // rsp/rsp.c, not necessarily in need of alignment
    RCPREG* CR[16];
    
    // rsp/vu/cf.h, all need alignment
    short ne[8]; /* $vco:  high byte "NOTEQUAL" */
    short co[8]; /* $vco:  low byte "carry/borrow in/out" */
    short clip[8]; /* $vcc:  high byte (clip tests:  VCL, VCH, VCR) */
    short comp[8]; /* $vcc:  low byte (VEQ, VNE, VLT, VGE, VCL, VCH, VCR) */
    short vce[8]; /* $vce:  vector compare extension register */
    
    // rsp_hle/mp3.c, let's see if aligning this helps anything
    uint8_t mp3data[0x1000];
    int32_t mp3_v[32];
    uint32_t mp3_inPtr, mp3_outPtr;
    uint32_t mp3_t6;/* = 0x08A0; - I think these are temporary storage buffers */
    uint32_t mp3_t5;/* = 0x0AC0; */
    uint32_t mp3_t4;/* = (w1 & 0x1E); */
    
    // All further members of the structure need not be aligned

    // rsp/vu/divrom.h
    int DivIn; /* buffered numerator of division read from vector file */
    int DivOut; /* global division result set by VRCP/VRCPL/VRSQ/VRSQH */
#if (0)
    int MovIn; /* We do not emulate this register (obsolete, for VMOV). */
#endif
    
    int DPH;
    
    // rsp/rsp.h
    int stage; // unused since EMULATE_STATIC_PC is defined by default in rsp/config.h
    int temp_PC;
    short MFC0_count[32];
    
    // rsp_hle/alist.c
    uint8_t BufferSpace[0x10000];
    
    // rsp_hle/alist_audio.c
    /* alist audio state */
    struct {
        /* segments */
        uint32_t segments[N_SEGMENTS];
        
        /* main buffers */
        uint16_t in;
        uint16_t out;
        uint16_t count;
        
        /* auxiliary buffers */
        uint16_t dry_right;
        uint16_t wet_left;
        uint16_t wet_right;
        
        /* gains */
        int16_t dry;
        int16_t wet;
        
        /* envelopes (0:left, 1:right) */
        int16_t vol[2];
        int16_t target[2];
        int32_t rate[2];
        
        /* ADPCM loop point address */
        uint32_t loop;
        
        /* storage for ADPCM table and polef coefficients */
        int16_t table[16 * 8];
    } l_alist_audio;

    struct {
        /* gains */
        int16_t dry;
        int16_t wet;
        
        /* envelopes (0:left, 1:right) */
        int16_t vol[2];
        int16_t target[2];
        int32_t rate[2];
        
        /* ADPCM loop point address */
        uint32_t loop;
        
        /* storage for ADPCM table and polef coefficients */
        int16_t table[16 * 8];
    } l_alist_naudio;
    
    struct {
        /* main buffers */
        uint16_t in;
        uint16_t out;
        uint16_t count;
        
        /* envmixer ramps */
        uint16_t env_values[3];
        uint16_t env_steps[3];
        
        /* ADPCM loop point address */
        uint32_t loop;
        
        /* storage for ADPCM table and polef coefficients */
        int16_t table[16 * 8];
        
        /* filter audio command state */
        uint16_t filter_count;
        uint32_t filter_lut_address[2];
    } l_alist_nead;

    uint32_t cpu_running, cpu_stopped;
    
    // options from file tags
    uint32_t enablecompare, enableFIFOfull;
    
    // options for decoding
    uint32_t enable_hle_audio;
    
    // buffering for rendered sample data
    size_t sample_buffer_count;
    int16_t * sample_buffer;

    // audio.c
    int32_t SampleRate;
    int16_t samplebuf[16384];
    size_t samples_in_buffer;
    
    const char * last_error;
    char error_message[1024];
    
    // cpu.c
    uint32_t NextInstruction, JumpToLocation, AudioIntrReg;
    CPU_ACTION * CPU_Action;
    SYSTEM_TIMERS * Timers;
    OPCODE Opcode;
    uint32_t CPURunning, SPHack;
    uint32_t * WaitMode;
    
    // interpreter_ops.c
    uint32_t SWL_MASK[4], SWR_MASK[4], LWL_MASK[4], LWR_MASK[4];
    int32_t SWL_SHIFT[4], SWR_SHIFT[4], LWL_SHIFT[4], LWR_SHIFT[4];
    int32_t RoundingModel;

    // memory.c
    uintptr_t *TLB_Map;
    uint8_t * MemChunk;
    uint32_t RdramSize, SystemRdramSize, RomFileSize;
    uint8_t * N64MEM, * RDRAM, * DMEM, * IMEM, * ROMPages[0x400], * savestatespace, * NOMEM;
    
    uint32_t WrittenToRom;
    uint32_t WroteToRom;
    uint32_t TempValue;
    uint32_t MemoryState;
    
    uint8_t EmptySpace;
    
    // pif.c
    uint8_t *PIF_Ram;
    
    // registers.c
    uint32_t PROGRAM_COUNTER, * CP0,*FPCR,*RegRDRAM,*RegSP,*RegDPC,*RegMI,*RegVI,*RegAI,*RegPI,
	*RegRI,*RegSI, HalfLine, RegModValue, ViFieldNumber, LLBit, LLAddr;
    void * FPRDoubleLocation[32], * FPRFloatLocation[32];
    MIPS_DWORD *GPR, *FPR, HI, LO;
    int32_t fpuControl;
    N64_REGISTERS * Registers;
    
    // tlb.c
    FASTTLB FastTlb[64];
    TLB tlb[32];
};

#define USF_STATE_HELPER ((usf_state_helper_t *)(state))

#define USF_STATE ((usf_state_t *)(((uint8_t *)(state))+((usf_state_helper_t *)(state))->offset_to_structure))

#endif
