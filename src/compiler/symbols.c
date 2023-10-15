/*
 * @symbols.c
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
#include <stdlib.h>
#include <string.h>

#include "compile.h"

// local function prototypes
static Symbol_t* AddLocalSymbol(ParseContext_t *c, SymbolTable_t *table, const char *name, StorageClass_t storageClass, Type_t *type, VMVALUE value);
static Symbol_t* FindSymbol(SymbolTable_t *table, const char *name);

// InitSymbolTable - initialize a symbol table
void InitSymbolTable(SymbolTable_t *table)
{
    table->head = NULL;
    table->pTail = &table->head;
    table->count = 0;
}

// AddGlobal - add a global symbol to the symbol table
Symbol_t* AddGlobal(ParseContext_t *c, const char *name, StorageClass_t storageClass, Type_t *type, VMVALUE value) {
    size_t size = sizeof(Symbol_t) + strlen(name);
    Symbol_t *sym;
    
    // allocate the symbol structure
    sym = (Symbol_t*) vm_allocate_low_memory(c->sys, size);
    strcpy(sym->name, name);
    sym->placed = VMFALSE;
    sym->storageClass = storageClass;
    sym->value = value;
    sym->next = NULL;

    // add it to the symbol table
    *c->globals.pTail = sym;
    c->globals.pTail = &sym->next;
    ++c->globals.count;
    
    // return the symbol
    return sym;
}

// AddArgument - add an argument symbol to symbol table
Symbol_t* AddArgument(ParseContext_t *c, const char *name, StorageClass_t storageClass, Type_t *type, VMVALUE value) {
    return AddLocalSymbol(c, &c->currentFunction->u.functionDefinition.arguments, name, storageClass, type, value);
}

// AddLocal - add a local symbol to the symbol table
Symbol_t* AddLocal(ParseContext_t *c, const char *name, StorageClass_t storageClass, Type_t *type, VMVALUE value) {
    return AddLocalSymbol(c, &c->currentFunction->u.functionDefinition.locals, name, storageClass, type, value);
}

// AddLocalSymbol - add a symbol to a local symbol table
static Symbol_t* AddLocalSymbol(ParseContext_t *c, SymbolTable_t *table, const char *name, StorageClass_t storageClass, Type_t *type, VMVALUE value) {
    size_t size = sizeof(Symbol_t) + strlen(name);
    Symbol_t *sym;
    
    // allocate the symbol structure
    sym = (Symbol_t*) system_allocate_high_memory(c->sys, size);
    strcpy(sym->name, name);
    sym->placed = VMTRUE;
    sym->storageClass = storageClass;
    sym->type = type;
    sym->value = value;
    sym->next = NULL;

    // add it to the symbol table
    *table->pTail = sym;
    table->pTail = &sym->next;
    ++table->count;
    
    // return the symbol
    return sym;
}

// FindGlobal - find a global symbol
Symbol_t* FindGlobal(ParseContext_t *c, const char *name) {
    return FindSymbol(&c->globals, name);
}

// FindArgument - find an argument symbol
Symbol_t* FindArgument(ParseContext_t *c, const char *name) {
    return FindSymbol(&c->currentFunction->u.functionDefinition.arguments, name);
}

// FindLocal - find an local symbol
Symbol_t* FindLocal(ParseContext_t *c, const char *name) {
    return FindSymbol(&c->currentFunction->u.functionDefinition.locals, name);
}

// FindSymbol - find a symbol in a symbol table
static Symbol_t* FindSymbol(SymbolTable_t *table, const char *name) {
    Symbol_t *sym = table->head;
    while (sym) {
        if (strcasecmp(name, sym->name) == 0)
            return sym;
        sym = sym->next;
    }
    return NULL;
}

// DumpSymbols - dump a symbol table
void DumpSymbols(SymbolTable_t *table, const char *tag) {
    Symbol_t *sym;
    if ((sym = table->head) != NULL) {
        vm_printf("%s:\n", tag);
        for (; sym != NULL; sym = sym->next)
            vm_printf("  %s %08x%s\n", sym->name, sym->value, sym->placed ? "" : " <undefined>");
    }
}
