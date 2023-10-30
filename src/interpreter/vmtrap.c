/*
 * @vmtrap.c
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

#include <stdint.h>
#include <stdbool.h>

#include "vm.h"

void vm_do_trap(vm_t *i, uint8_t op) {
    switch (op) {
        case TRAP_GetChar:
            vm_push(i, i->tos);
            i->tos = vm_getchar();
            break;
        case TRAP_PutChar:
            vm_putchar(i->tos);
            i->tos = vm_pop(i);
            break;
        case TRAP_PrintStr:
            vm_printf("%s", (char*) (i->code + i->tos));
            i->tos = *i->sp++;
            break;
        case TRAP_PrintInt:
            vm_printf("%d", i->tos);
            i->tos = *i->sp++;
            break;
        case TRAP_PrintTab:
            vm_putchar('\t');
            break;
        case TRAP_PrintNL:
            vm_putchar('\n');
            break;
        case TRAP_PrintFlush:
            vm_flush();
            break;
        default:
            vm_abort(i, "undefined print opcode 0x%02x", op);
            break;
    }
}
