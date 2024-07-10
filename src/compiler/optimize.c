/*
 * @optimize.c
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
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "compile.h"
#include "vmdebug.h"

extern int generate_functionCount;
extern functions_t generate_functions[100];

typedef struct codeString_s {
    uint32_t pos;
       char *str;
} codeString_t;

typedef struct codeGlobal_s {
    uint32_t pos;
        char *name;
        bool placed;
     VMVALUE value;
} codeGlobal_t;

typedef struct codeFunction_s {
       char *name;
       char **code;
    uint32_t code_len;
} codeFunction_t;

typedef struct codeActual_s {
      codeString_t *strings;
          uint32_t strings_qty;
      codeGlobal_t *globals;
          uint32_t globals_qty;
    codeFunction_t *functions;
          uint32_t functions_qty;
} codeActual_t;

codeActual_t actual;

void optimize(ParseContext_t *c, bool dump) {
    uint32_t i, j;

    actual.strings = malloc(sizeof(codeString_t));
    actual.strings_qty = 0;
    actual.globals = malloc(sizeof(codeGlobal_t));
    actual.globals_qty = 0;
    actual.functions = malloc(sizeof(codeFunction_t));
    actual.functions_qty = 0;

    // load strings
    String_t *str = c->strings;
    if (str) {
        while (str != NULL) {
            actual.strings = realloc(actual.strings, (actual.strings_qty + 1) * sizeof(codeString_t));
            actual.strings[actual.strings_qty].str = strdup(str->data);
            actual.strings[actual.strings_qty].pos = (VMVALUE) ((uint8_t*) str->data - c->g->codeBuf);
            ++actual.strings_qty;
            str = str->next;
        }
    }

    // load globals
    Symbol_t *sym;
    if ((sym = c->globals.head) != NULL) {
        for (; sym != NULL; sym = sym->next) {
            actual.globals = realloc(actual.globals, (actual.globals_qty + 1) * sizeof(codeGlobal_t));
            actual.globals[actual.globals_qty].name = strdup(sym->name);
            actual.globals[actual.globals_qty].pos= sym->value;
            actual.globals[actual.globals_qty].placed = (bool)sym->placed;
            actual.globals[actual.globals_qty].value = sym->value;
            ++actual.globals_qty;
        }
    }

    for (i = 0; i < generate_functionCount; ++i) {
        actual.functions = realloc(actual.functions, (actual.functions_qty + 1) * sizeof(codeFunction_t));
        actual.functions[actual.functions_qty].code = malloc(sizeof(char*));
        actual.functions[actual.functions_qty].code_len = 0;

        actual.functions[actual.functions_qty].name = strdup(generate_functions[i].symbol ? generate_functions[i].symbol->name : "<main>");
        vmdebug_decode_function(generate_functions[i].code, c->g->codeBuf + generate_functions[i].code, generate_functions[i].codeLen, &(actual.functions[actual.functions_qty].code),
                &actual.functions[actual.functions_qty].code_len, true);

        ++actual.functions_qty;
    }

    // show actual
    if (dump) {
        vm_printf("STRINGS:\n");
        for (i = 0; i < actual.strings_qty; i++) {
            vm_printf("  %s\n", actual.strings[i].str);
        }

        vm_printf("\nGLOBALS:\n");
        for (i = 0; i < actual.globals_qty; i++) {
            vm_printf("  %s: %d(%08x) %s\n", actual.globals[i].name, actual.globals[i].value, actual.globals[i].value, actual.globals[i].placed ? "(placed)" : "");
        }

        vm_printf("\nFUNCTIONS:\n");
        for (i = 0; i < actual.functions_qty; i++) {
            vm_printf("  %s:\n", actual.functions[i].name);
            for (j = 0; j < actual.functions[i].code_len; j++) {
                vm_printf("    %s\n", actual.functions[i].code[j]);
            }
            vm_printf("\n");
        }
    } else {
    // optimize


    }

    // free
    for (i = 0; i < actual.strings_qty; i++) {
        free(actual.strings[i].str);
    }
    free(actual.strings);

    for (i = 0; i < actual.globals_qty; i++) {
        free(actual.globals[i].name);
    }
    free(actual.globals);

    for (i = 0; i < actual.functions_qty; i++) {
        free(actual.functions[i].name);
        for (j = 0; j < actual.functions[i].code_len; j++) {
            free(actual.functions[i].code[j]);
        }
        free(actual.functions[i].code);
    }
    free(actual.functions);
}
