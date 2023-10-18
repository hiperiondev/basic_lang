/*
 * @compile.c
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

#include "compile.h"
#include "vm.h"

// InitCompileContext - initialize the compile (parse) context
ParseContext_t* InitCompileContext(vm_context_t *sys) {
    ParseContext_t *c;
    
    if (!(c = InitParseContext(sys))
            || !(c->g = InitGenerateContext(sys)))
        vm_printf("insufficient memory");
    return c;
}

// Compile - parse a program
void Compile(ParseContext_t *c) {
    VMVALUE mainCode;
    vm_t *i;
    Symbol_t *symbol;
    
    uint8_t *init_mem = c->sys->nextLow;

    // setup an error target
    if (setjmp(c->sys->errorTarget) != 0)
        return;

    // initialize the string table
    c->strings = NULL;

    // initialize block nesting table
    c->btop = (Block_t*) ((char*) c->blockBuf + sizeof(c->blockBuf));
    c->bptr = &c->blockBuf[0] - 1;

    // create the main function
    c->mainFunction = StartFunction(c, NULL);
    
    // initialize the global symbol table
    InitSymbolTable(&c->globals);
    
    // initialize scanner
    InitScan(c);
    
    // parse the program
    while (ParseGetLine(c)) {
        int tkn;
        if ((tkn = GetToken(c)) != T_EOL)
            ParseStatement(c, tkn);
    }
    
    PrintNode(c->mainFunction, 0);
    
    // generate code for the main function
    mainCode = Generate(c->g, c->mainFunction);
    
    uint32_t code_len = (c->sys->nextLow - init_mem);

    // store all implicitly declared global variables
    for (symbol = c->globals.head; symbol != NULL; symbol = symbol->next) {
        if (symbol->storageClass == SC_VARIABLE && !symbol->placed) {
            VMVALUE value = 0;
            VMVALUE addr = StoreVector(c->g, &value, 1);
            PlaceSymbol(c->g, symbol, addr);
        }
    }

    DumpFunctions(c->g);
    DumpSymbols(&c->globals, "Globals");
    DumpStrings(c);
    

    if (!(i = vm_init(c->g->codeBuf, code_len, 1024)))
        vm_printf("insufficient memory");
    else {
        vm_execute(i, mainCode);
        vm_deinit(i);
    }
}

// PushFile - push a file onto the input file stack
int PushFile(ParseContext_t *c, const char *name) {
    vm_context_t *sys = c->sys;
    IncludedFile_t *inc;
    ParseFile_t *f;
    void *fp;
    
    // check to see if the file has already been included
    for (inc = c->includedFiles; inc != NULL; inc = inc->next)
        if (strcmp(name, inc->name) == 0)
            return VMTRUE;
    
    // add this file to the list of already included files
    if (!(inc = (IncludedFile_t*) system_allocate_high_memory(sys, sizeof(IncludedFile_t) + strlen(name))))
        vm_system_abort(sys, "insufficient memory");
    strcpy(inc->name, name);
    inc->next = c->includedFiles;
    c->includedFiles = inc;

    // open the input file
    if (!(fp = system_fs_open(sys, name, "r")))
        return VMFALSE;
    
    // allocate a parse file structure
    if (!(f = (ParseFile_t*) system_allocate_high_memory(sys, sizeof(ParseFile_t))))
        vm_system_abort(sys, "insufficient memory");
    
    // initialize the parse file structure
    f->fp = fp;
    f->file = inc;
    f->lineNumber = 0;
    
    // push the file onto the input file stack
    f->next = c->currentFile;
    c->currentFile = f;
    
    // return successfully
    return VMTRUE;
}

// ParseGetLine - get the next input line
int ParseGetLine(ParseContext_t *c) {
    System_line_t *sys = c->sys_line;
    ParseFile_t *f;
    int len;

    // get the next input line
    for (;;) {
        
        // get a line from the main input
        if (!(f = c->currentFile)) {
            if (system_get_line(sys, &c->lineNumber))
                break;
            else
                return VMFALSE;
        }

        // get a line from the current include file
        else {
            if (system_fs_getline(sys->lineBuf, sizeof(sys->lineBuf) - 1, f->fp)) {
                c->lineNumber = ++f->lineNumber;
                sys->linePtr = sys->lineBuf;
                break;
            }
            else {
                c->currentFile = f->next;
                system_fs_close(f->fp);
            }
        }
    }
    
    // make sure the line is correctly terminated
    len = strlen(sys->lineBuf);
    if (len == 0 || sys->lineBuf[len - 1] != '\n') {
        sys->lineBuf[len++] = '\n';
        sys->lineBuf[len] = '\0';
    }

    // return successfully
    return VMTRUE;
}
