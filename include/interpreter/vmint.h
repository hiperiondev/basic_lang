/*
 * @vmint.h
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

#ifndef __VMINT_H__
#define __VMINT_H__

#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>

#include "vmsystem.h"
#include "vmimage.h"

// forward type declarations
typedef struct Interpreter_s Interpreter_t;

// intrinsic function handler type
typedef void IntrinsicFcn(Interpreter_t *i);

// interpreter state structure
struct Interpreter_s {
    System_t *sys;
     uint8_t *base;
     jmp_buf errorTarget;
     VMVALUE *stack;
     VMVALUE *stackTop;
     uint8_t *pc;
     VMVALUE *fp;
     VMVALUE *sp;
     VMVALUE tos;
};

// stack frame offsets
#define F_FP    -1
#define F_SIZE  1

// stack manipulation macros
#define Reserve(i, n)   do {                                    \
                            if ((i)->sp - (n) < (i)->stack)     \
                                StackOverflow(i);               \
                            else  {                             \
                                int _cnt = (n);                 \
                                while (--_cnt >= 0)             \
                                    Push(i, 0);                 \
                            }                                   \
                        } while (0)
#define CPush(i, v)     do {                                    \
                            if ((i)->sp - 1 < (i)->stack)       \
                                StackOverflow(i);               \
                            else                                \
                                Push(i, v);                     \
                        } while (0)
#define Push(i, v)      (*--(i)->sp = (v))
#define Pop(i)          (*(i)->sp++)
#define Top(i)          (*(i)->sp)
#define Drop(i, n)      ((i)->sp += (n))

// prototypes for xbint.c
          void Fatal(System_t *sys, const char *fmt, ...);

// prototypes from db_vmint.c
Interpreter_t* InitInterpreter(System_t *sys, uint8_t *base, int stackSize);
           int Execute(Interpreter_t *i, VMVALUE mainCode);
          void AbortVM(Interpreter_t *i, const char *fmt, ...);
          void StackOverflow(Interpreter_t *i);
          void ShowStack(Interpreter_t *i);

// prototypes and variables from db_vmfcn.c
extern IntrinsicFcn *Intrinsics[];
extern int IntrinsicCount;

#endif
