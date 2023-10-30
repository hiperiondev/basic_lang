/*
 * @vmdebug.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "vmopcodes.h"
#include "vmdebug.h"
#include "vmsystem.h"
#include <ctype.h>

otdef_t opcode_table[] = {
        { OP_HALT,    "HALT",    FMT_NONE   },
        { OP_BRT,     "BRT",     FMT_BR     },
        { OP_BRTSC,   "BRTSC",   FMT_BR     },
        { OP_BRF,     "BRF",     FMT_BR     },
        { OP_BRFSC,   "BRFSC",   FMT_BR     },
        { OP_BR,      "BR",      FMT_BR     },
        { OP_NOT,     "NOT",     FMT_NONE   },
        { OP_NEG,     "NEG",     FMT_NONE   },
        { OP_ADD,     "ADD",     FMT_NONE   },
        { OP_SUB,     "SUB",     FMT_NONE   },
        { OP_MUL,     "MUL",     FMT_NONE   },
        { OP_DIV,     "DIV",     FMT_NONE   },
        { OP_REM,     "REM",     FMT_NONE   },
        { OP_BNOT,    "BNOT",    FMT_NONE   },
        { OP_BAND,    "BAND",    FMT_NONE   },
        { OP_BOR,     "BOR",     FMT_NONE   },
        { OP_BXOR,    "BXOR",    FMT_NONE   },
        { OP_SHL,     "SHL",     FMT_NONE   },
        { OP_SHR,     "SHR",     FMT_NONE   },
        { OP_LT,      "LT",      FMT_NONE   },
        { OP_LE,      "LE",      FMT_NONE   },
        { OP_EQ,      "EQ",      FMT_NONE   },
        { OP_NE,      "NE",      FMT_NONE   },
        { OP_GE,      "GE",      FMT_NONE   },
        { OP_GT,      "GT",      FMT_NONE   },
        { OP_LIT,     "LIT",     FMT_WORD   },
        { OP_SLIT,    "SLIT",    FMT_SBYTE  },
        { OP_LOAD,    "LOAD",    FMT_NONE   },
        { OP_LOADB,   "LOADB",   FMT_NONE   },
        { OP_STORE,   "STORE",   FMT_NONE   },
        { OP_STOREB,  "STOREB",  FMT_NONE   },
        { OP_LREF,    "LREF",    FMT_SBYTE  },
        { OP_LSET,    "LSET",    FMT_SBYTE  },
        { OP_INDEX,   "INDEX",   FMT_NONE   },
        { OP_CALL,    "CALL",    FMT_NONE   },
        { OP_FRAME,   "FRAME",   FMT_BYTE   },
        { OP_RETURN,  "RETURN",  FMT_NONE   },
        { OP_RETURNZ, "RETURNZ", FMT_NONE   },
        { OP_CLEAN,   "CLEAN",   FMT_BYTE   },
        { OP_DROP,    "DROP",    FMT_NONE   },
        { OP_DUP,     "DUP",     FMT_NONE   },
        { OP_NATIVE,  "NATIVE",  FMT_NATIVE },
        { OP_TRAP,    "TRAP",    FMT_BYTE   },
        { OP_RETURN,  "RETURNX", FMT_NONE   },  // RETURN is an basic keyword
        { 0, NULL, 0 }
};

static char* rtrim(char *s) {
    char *back = s + strlen(s);
    while (isspace(*--back));
    *(back + 1) = '\0';
    return s;
}

// DecodeFunction - decode the instructions in a function code object
void vmdebug_decode_function(VMUVALUE base, const uint8_t *code, int len, char ***asmcode, uint32_t *asmcode_qty, bool toCode) {
    char *opcode = NULL;
    const uint8_t *end = code + len;

    while (code < end) {
        int len = vmdebug_decode_instruction(base, code, &opcode, toCode);

        if (toCode) {
            if (opcode != NULL) {
                (*asmcode) = realloc((*asmcode), ((*asmcode_qty) + 1) * sizeof(char*));
                (*asmcode)[*asmcode_qty] = malloc(256 * sizeof(char));
                strcpy((*asmcode)[*asmcode_qty], opcode);
                ++(*asmcode_qty);
            } else
                printf(" > NOCODE\n");
        } else
            printf(" >> %s\n", opcode);

        free(opcode);
        opcode = NULL;

        code += len;
        base += len;
    }
}

#define toStr(str, tmp, fmt, ...)                 \
            {                                     \
		        sprintf(tmp, fmt, ##__VA_ARGS__); \
		        strcat(str, tmp);                 \
		        tmp[0] = '\0';                    \
            }

// DecodeInstruction - decode a single bytecode instruction
int vmdebug_decode_instruction(VMUVALUE addr, const uint8_t *lc, char **str_code, bool toCode) {
    uint8_t opcode, bytes[sizeof(VMVALUE)];
    VMVALUE offset = 0;
    int8_t sbyte;
    otdef_t *op;
    int n, i;
    char str[1024] = { 0 };
    char tmp[1024] = { 0 };

    // get the opcode
    opcode = VMCODEBYTE(lc);

    // show the address
    if (!toCode)
        toStr(str, tmp, "%0*x %02x ", (int)sizeof(VMVALUE) * 2, addr, opcode);
    n = 1;

    // display the operands
    for (op = opcode_table; op->name; ++op)
        if (opcode == op->code) {
            switch (op->fmt) {
                case FMT_NONE:
                    for (i = 0; i < sizeof(VMVALUE); ++i) {
                        if (!toCode)
                            toStr(str, tmp, "   ");
                    }

                    toStr(str, tmp, "%s\n", op->name);
                    break;
                case FMT_BYTE:
                    bytes[0] = VMCODEBYTE(lc + 1);

                    if (!toCode)
                        toStr(str, tmp, "%02x ", bytes[0]);
                    for (i = 1; i < sizeof(VMVALUE); ++i)
                        if (!toCode)
                            toStr(str, tmp, "   ");

                    toStr(str, tmp, "%s %02x\n", op->name, bytes[0]);
                    n += 1;
                    break;
                case FMT_SBYTE:
                    sbyte = (int8_t) VMCODEBYTE(lc + 1);

                    if (!toCode)
                        toStr(str, tmp, "%02x ", (uint8_t ) sbyte);
                    for (i = 1; i < sizeof(VMVALUE); ++i)
                        if (!toCode)
                            toStr(str, tmp, "   ");

                    toStr(str, tmp, "%s %d\n", op->name, sbyte);
                    n += 1;
                    break;
                case FMT_WORD:
                    case FMT_NATIVE:
                    for (i = 0; i < sizeof(VMVALUE); ++i) {
                        bytes[i] = VMCODEBYTE(lc + i + 1);
                        if (!toCode)
                            toStr(str, tmp, "%02x ", bytes[i]);
                    }
                    toStr(str, tmp, "%s ", op->name);

                    for (i = 0; i < sizeof(VMVALUE); ++i)
                        toStr(str, tmp, "%02x", bytes[i]);
                    toStr(str, tmp, "\n");
                    n += sizeof(VMVALUE);
                    break;
                case FMT_BR:
                    for (i = 0; i < sizeof(VMVALUE); ++i) {
                        bytes[i] = VMCODEBYTE(lc + i + 1);
                        offset = (offset << 8) | bytes[i];
                        if (!toCode)
                            toStr(str, tmp, "%02x ", bytes[i]);
                    }

                    toStr(str, tmp, "%s ", op->name);
                    for (i = 0; i < sizeof(VMVALUE); ++i)
                        toStr(str, tmp, "%02x", bytes[i]);
                    if (!toCode)
                        toStr(str, tmp, " # %04x\n", (int)(addr + 1 + sizeof(VMVALUE) + offset));
                    n += sizeof(VMVALUE);
                    break;
            }

            *str_code = strdup(str);
            *str_code = rtrim(*str_code);

            return n;
        }

    // unknown opcode
    toStr(str, tmp, "      <UNKNOWN>\n");
    *str_code = strdup(str);
    *str_code = rtrim(*str_code);

    return 1;
}

void vm_show_stack(vm_t *i) {
    VMVALUE *p;
    if (i->sp < i->stackTop) {
        vm_printf(" %d", i->tos);
        for (p = i->sp; p < i->stackTop - 1; ++p) {
            if (p == i->fp)
                vm_printf(" <fp>");
            vm_printf(" %d", *p);
        }
        vm_printf("\n");
    }
}
