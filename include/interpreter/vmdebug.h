/*
 * @vmdebug.h
 *
 * @brief
 * @details
 * This is based on other projects:
 *   junkbasic (David Michael Betz): https://github.com/dbetz/junkbasic/
 *   Others (see individual files)
 *
 *   please contact their authors for more information.
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/basic_lang
 */

#ifndef __VMDEBUG_H__
#define __VMDEBUG_H__

#include "vmsystem.h"

// instruction output formats
#define FMT_NONE    0
#define FMT_BYTE    1
#define FMT_SBYTE   2
#define FMT_WORD    3
#define FMT_NATIVE  4
#define FMT_BR      5

typedef struct {
     int code;
    char *name;
     int fmt;
} OTDEF_t;

extern OTDEF_t OpcodeTable[];

void DecodeFunction(VMUVALUE base, const uint8_t *code, int len);
 int DecodeInstruction(VMUVALUE addr, const uint8_t *lc);

#endif
