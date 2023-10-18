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
#include "vmopcodes.h"

//#define VM_DEBUG
#define VM_TRAP

// vm trap codes
enum {
    TRAP_GetChar    = 0,
    TRAP_PutChar    = 1,
    TRAP_PrintStr   = 2,
    TRAP_PrintInt   = 3,
    TRAP_PrintTab   = 4,
    TRAP_PrintNL    = 5,
    TRAP_PrintFlush = 6,
};

// interpreter state structure
typedef struct vm_s {
         uint8_t *base;
         jmp_buf errorTarget;
         VMVALUE *stack;
         VMVALUE *stackTop;
         uint8_t *pc;
         VMVALUE *fp;
         VMVALUE *sp;
         VMVALUE tos;
} vm_t;

// stack frame offsets
#define F_FP    -1

// stack manipulation macros
#define vm_reserve(i, n)  if ((i)->sp - (n) < (i)->stack) \
                              vm_stack_overflow(i);       \
                          else  {                         \
                              int _cnt = (n);             \
                              while (--_cnt >= 0)         \
                              vm_push(i, 0);              \
                          }

#define vm_cpush(i, v)    if ((i)->sp - 1 < (i)->stack)   \
                              vm_stack_overflow(i);       \
                          else                            \
                              vm_push(i, v);

#define vm_push(i, v)     (*--(i)->sp = (v))
#define vm_pop(i)         (*(i)->sp++)
#define vm_top(i)         (*(i)->sp)
#define vm_drop(i, n)     ((i)->sp += (n))

// prototypes from db_vmint.c
  vm_t* vm_initialize(vm_context_t *sys, uint8_t *base, VMVALUE stackSize);
uint8_t vm_execute(vm_t *i, VMVALUE mainCode);
   void vm_abort(vm_t *i, const char *fmt, ...);
   void vm_stack_overflow(vm_t *i);

// prototypes and variables
   typedef void vm_intrinsic_func(vm_t *i);
         extern vm_intrinsic_func *vm_intrinsics[];
extern uint32_t vm_intrinsic_cnt;

#endif
