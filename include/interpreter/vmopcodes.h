/*
 * @vmimage.h
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

#ifndef __VMIMAGE_H__
#define __VMIMAGE_H__

#include <stdarg.h>

#include "vmtypes.h"

// opcodes
enum OPCODES {
    OP_HALT,    // 0x00 halt
    OP_BRT,     // 0x01 branch on true
    OP_BRTSC,   // 0x02 branch on true (for short circuit booleans)
    OP_BRF,     // 0x03 branch on false
    OP_BRFSC,   // 0x04 branch on false (for short circuit booleans)
    OP_BR,      // 0x05 branch unconditionally
    OP_NOT,     // 0x06 logical negate top of stack
    OP_NEG,     // 0x07 negate
    OP_ADD,     // 0x08 add two numeric expressions
    OP_SUB,     // 0x09 subtract two numeric expressions
    OP_MUL,     // 0x0a multiply two numeric expressions
    OP_DIV,     // 0x0b divide two numeric expressions
    OP_REM,     // 0x0c remainder of two numeric expressions
    OP_BNOT,    // 0x0d bitwise not of two numeric expressions
    OP_BAND,    // 0x0e bitwise and of two numeric expressions
    OP_BOR,     // 0x0f bitwise or of two numeric expressions
    OP_BXOR,    // 0x10 bitwise exclusive or
    OP_SHL,     // 0x11 shift left
    OP_SHR,     // 0x12 shift right
    OP_LT,      // 0x13 less than
    OP_LE,      // 0x14 less than or equal to
    OP_EQ,      // 0x15 equal to
    OP_NE,      // 0x16 not equal to
    OP_GE,      // 0x17 greater than or equal to
    OP_GT,      // 0x18 greater than
    OP_LIT,     // 0x19 load a literal
    OP_SLIT,    // 0x1a load a short literal (-128 to 127)
    OP_LOAD,    // 0x1b load a long from memory
    OP_LOADB,   // 0x1c load a byte from memory
    OP_STORE,   // 0x1d store a long into memory
    OP_STOREB,  // 0x1e store a byte into memory
    OP_LREF,    // 0x1f load a local variable relative to the frame pointer
    OP_LSET,    // 0x20 set a local variable relative to the frame pointer
    OP_INDEX,   // 0x21 index into a vector of longs
    OP_CALL,    // 0x22 call a function
    OP_FRAME,   // 0x23 create a stack frame
    OP_RETURN,  // 0x24 remove a stack frame and return from a function call
    OP_DROP,    // 0x25 drop the top element of the stack
    OP_DUP,     // 0x26 duplicate the top element of the stack
    OP_NATIVE,  // 0x27 execute native code
    OP_TRAP,    // 0x28 trap to handler
    OP_RETURNZ, // 0x29
    OP_CLEAN    // 0x2c drop n elements
};

#endif
