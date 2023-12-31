/*
 * @vmint.c
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

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "vmopcodes.h"
#include "vm.h"
#include "vmsystem.h"

#ifdef VM_DEBUG
#include "vmdebug.h"
#endif

#ifdef VM_TRAP
#include "vmtrap.h"
#endif

void vm_abort(vm_t *i, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vm_printf("abort: ");
    vm_printf(fmt, ap);
    vm_printf("\n");
    va_end(ap);
    if (i)
        longjmp(i->errorTarget, 1);
}

// initialize the interpreter
vm_t* vm_init(uint8_t *code, uint32_t code_len, VMVALUE stackSize, bool reference_code) {
    vm_t *i;

    if (!(i = (vm_t*) malloc(sizeof(vm_t))))
        return NULL;

    if (!(i->stack = (VMVALUE*) malloc(stackSize * sizeof(VMVALUE))))
        return NULL;

    i->stackTop = i->stack + stackSize;
    i->codelen = code_len;
    i->stack_size = stackSize;

    if (reference_code) {
        i->code = code;
        i->code_referenced = true;
    } else {
        i->code = malloc(code_len * sizeof(uint8_t));
        if (code != NULL)
            memcpy(i->code, code, code_len);
        i->code_referenced = false;
    }

    return i;
}

void vm_deinit(vm_t *i) {
    if (i == NULL)
        return;

    free(i->stack);
    if (!i->code_referenced)
        free(i->code);
    free(i);

}

// execute the main code
uint8_t vm_execute(vm_t *i, VMVALUE mainCode) {
    VMVALUE tmp;
    int8_t tmpb;
    int32_t cnt;

    // initialize
    i->pc = i->code + mainCode;
    i->sp = i->fp = i->stackTop;

    if (setjmp(i->errorTarget))
        return VMFALSE;

    for (;;) {
#ifdef VM_DEBUG
        vm_show_stack(i);
        vmdebug_decode_instruction(i->pc - i->code, i->pc);
#endif
        switch (VMCODEBYTE(i->pc++)) {
            case OP_HALT:
                return VMTRUE;
            case OP_BRT:
                for (tmp = 0, cnt = sizeof(VMUVALUE); --cnt >= 0;)
                    tmp = (tmp << 8) | VMCODEBYTE(i->pc++);
                if (i->tos)
                    i->pc += tmp;
                i->tos = vm_pop(i);
                break;
            case OP_BRTSC:
                for (tmp = 0, cnt = sizeof(VMUVALUE); --cnt >= 0;)
                    tmp = (tmp << 8) | VMCODEBYTE(i->pc++);
                if (i->tos)
                    i->pc += tmp;
                else
                    i->tos = vm_pop(i);
                break;
            case OP_BRF:
                for (tmp = 0, cnt = sizeof(VMUVALUE); --cnt >= 0;)
                    tmp = (tmp << 8) | VMCODEBYTE(i->pc++);
                if (!i->tos)
                    i->pc += tmp;
                i->tos = vm_pop(i);
                break;
            case OP_BRFSC:
                for (tmp = 0, cnt = sizeof(VMUVALUE); --cnt >= 0;)
                    tmp = (tmp << 8) | VMCODEBYTE(i->pc++);
                if (!i->tos)
                    i->pc += tmp;
                else
                    i->tos = vm_pop(i);
                break;
            case OP_BR:
                for (tmp = 0, cnt = sizeof(VMUVALUE); --cnt >= 0;)
                    tmp = (tmp << 8) | VMCODEBYTE(i->pc++);
                i->pc += tmp;
                break;
            case OP_NOT:
                i->tos = (i->tos ? VMFALSE : VMTRUE);
                break;
            case OP_NEG:
                i->tos = -i->tos;
                break;
            case OP_ADD:
                tmp = vm_pop(i);
                i->tos = tmp + i->tos;
                break;
            case OP_SUB:
                tmp = vm_pop(i);
                i->tos = tmp - i->tos;
                break;
            case OP_MUL:
                tmp = vm_pop(i);
                i->tos = tmp * i->tos;
                break;
            case OP_DIV:
                tmp = vm_pop(i);
                i->tos = (i->tos == 0 ? 0 : tmp / i->tos);
                break;
            case OP_REM:
                tmp = vm_pop(i);
                i->tos = (i->tos == 0 ? 0 : tmp % i->tos);
                break;
            case OP_BNOT:
                i->tos = ~i->tos;
                break;
            case OP_BAND:
                tmp = vm_pop(i);
                i->tos = tmp & i->tos;
                break;
            case OP_BOR:
                tmp = vm_pop(i);
                i->tos = tmp | i->tos;
                break;
            case OP_BXOR:
                tmp = vm_pop(i);
                i->tos = tmp ^ i->tos;
                break;
            case OP_SHL:
                tmp = vm_pop(i);
                i->tos = tmp << i->tos;
                break;
            case OP_SHR:
                tmp = vm_pop(i);
                i->tos = tmp >> i->tos;
                break;
            case OP_LT:
                tmp = vm_pop(i);
                i->tos = (tmp < i->tos ? VMTRUE : VMFALSE);
                break;
            case OP_LE:
                tmp = vm_pop(i);
                i->tos = (tmp <= i->tos ? VMTRUE : VMFALSE);
                break;
            case OP_EQ:
                tmp = vm_pop(i);
                i->tos = (tmp == i->tos ? VMTRUE : VMFALSE);
                break;
            case OP_NE:
                tmp = vm_pop(i);
                i->tos = (tmp != i->tos ? VMTRUE : VMFALSE);
                break;
            case OP_GE:
                tmp = vm_pop(i);
                i->tos = (tmp >= i->tos ? VMTRUE : VMFALSE);
                break;
            case OP_GT:
                tmp = vm_pop(i);
                i->tos = (tmp > i->tos ? VMTRUE : VMFALSE);
                break;
            case OP_LIT:
                for (tmp = 0, cnt = sizeof(VMUVALUE); --cnt >= 0;)
                    tmp = (tmp << 8) | VMCODEBYTE(i->pc++);
                vm_cpush(i, i->tos);
                i->tos = tmp;
                break;
            case OP_SLIT:
                tmpb = (int8_t) VMCODEBYTE(i->pc++);
                vm_cpush(i, i->tos);
                i->tos = tmpb;
                break;
            case OP_LOAD:
                i->tos = *(VMVALUE*) (i->code + i->tos);
                break;
            case OP_LOADB:
                i->tos = *(i->code + i->tos);
                break;
            case OP_STORE:
                tmp = vm_pop(i);
                *(VMVALUE*) (i->code + i->tos) = tmp;
                i->tos = vm_pop(i);
                break;
            case OP_STOREB:
                tmp = vm_pop(i);
                *(i->code + i->tos) = tmp;
                i->tos = vm_pop(i);
                break;
            case OP_LREF:
                tmpb = (int8_t) VMCODEBYTE(i->pc++);
                vm_cpush(i, i->tos);
                i->tos = i->fp[(int) tmpb];
                break;
            case OP_LSET:
                tmpb = (int8_t) VMCODEBYTE(i->pc++);
                i->fp[(int) tmpb] = i->tos;
                i->tos = vm_pop(i);
                break;
            case OP_INDEX:
                tmp = vm_pop(i);
                i->tos = tmp + i->tos * sizeof(VMVALUE);
                break;
            case OP_CALL:
                tmp = (VMVALUE) (i->pc - (uint8_t*) i->code);
                i->pc = i->code + i->tos;
                i->tos = tmp;
                break;
            case OP_CLEAN:
                cnt = VMCODEBYTE(i->pc++);
                vm_drop(i, cnt);
                break;
            case OP_FRAME:
                cnt = VMCODEBYTE(i->pc++);
                tmp = (VMVALUE) (i->fp - i->stack);
                i->fp = i->sp;
                vm_reserve(i, cnt);
                i->fp[F_FP] = tmp;
                break;
            case OP_RETURNZ:
                vm_cpush(i, i->tos);
                i->tos = 0;
                //no break
            case OP_RETURN:
                i->pc = (uint8_t*) i->code + vm_top(i);
                i->sp = i->fp;
                i->fp = (VMVALUE*) (i->stack + i->fp[F_FP]);
                break;
            case OP_DROP:
                i->tos = vm_pop(i);
                break;
            case OP_DUP:
                vm_cpush(i, i->tos);
                break;
            case OP_NATIVE:
                for (tmp = 0, cnt = sizeof(VMUVALUE); --cnt >= 0;)
                    tmp = (tmp << 8) | VMCODEBYTE(i->pc++);
                break;
            case OP_TRAP:
#ifdef VM_TRAP
                vm_do_trap(i, VMCODEBYTE(i->pc++));
#endif
                break;
            default:
                vm_abort(i, "undefined opcode 0x%02x", VMCODEBYTE(i->pc - 1));
                break;
        }
    }

    return 0;
}
