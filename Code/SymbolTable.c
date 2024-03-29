#include "SymbolTable.h"
#include "SemanticAnalysis.h"

int stackSize;
int stackTop;
HashBucket symbolTable[MAX_ST_SIZE];
StackEle* symbolStack;
extern DataType* intSpecifier;
extern DataType* floatSpecifier;
extern DataType* errorSpecifier;
extern Symbol* fieldSymbol;
extern Symbol* errorSymbol;

unsigned int hash_pjw(char* name) {
    unsigned int val = 0, i;
    for (; *name; ++name) {
        val = (val << 2) + *name;
        if ((i = val & ~(MAX_ST_SIZE-1))) {
            val = (val ^ (i >> 12)) & (MAX_ST_SIZE-1);
        }
    }
    return val;
}

void init() {
    stackSize = 128;
    stackTop = 0;
    symbolStack = (StackEle*)malloc(sizeof(StackEle)*stackSize);
    memset(symbolTable, 0, MAX_ST_SIZE * sizeof(HashBucket));
    memset(symbolStack, 0, stackSize * sizeof(StackEle));
    intSpecifier = (DataType*)malloc(sizeof(DataType));
    intSpecifier->kind = DT_BASIC;
    intSpecifier->basic = BASIC_INT;
    floatSpecifier = (DataType*)malloc(sizeof(DataType));
    floatSpecifier->kind = DT_BASIC;
    floatSpecifier->basic = BASIC_FLOAT;
    errorSpecifier = (DataType*)malloc(sizeof(DataType));
    fieldSymbol = (Symbol*)malloc(sizeof(Symbol));
    errorSymbol = (Symbol*)malloc(sizeof(Symbol));
    Symbol* readFuncSymbol = NULL;
    Symbol* writeFuncSymbol = NULL;
    SMTC_SET_SYMBOL(readFuncSymbol, NS_FUNC)
    readFuncSymbol->name = (char*)malloc(sizeof(char)*8);
    strcpy(readFuncSymbol->name, "read");
    readFuncSymbol->funcData = (FuncData*)malloc(sizeof(FuncData));
    readFuncSymbol->funcData->retType = intSpecifier;
    readFuncSymbol->funcData->paramList = NULL;
    readFuncSymbol->funcData->tail = NULL;
    SMTC_SET_SYMBOL(writeFuncSymbol, NS_FUNC)
    writeFuncSymbol->name = (char*)malloc(sizeof(char)*8);
    strcpy(writeFuncSymbol->name, "write");
    writeFuncSymbol->funcData = (FuncData*)malloc(sizeof(FuncData));
    writeFuncSymbol->funcData->retType = intSpecifier;
    Field* writeParam = NULL;
    SMTC_SET_FIELD(writeParam)
    writeParam->name = NULL;
    writeParam->dataType = intSpecifier;
    writeFuncSymbol->funcData->paramList = writeParam;
    writeFuncSymbol->funcData->tail = writeParam;
    insertSymbol(readFuncSymbol);
    insertSymbol(writeFuncSymbol);
}

Symbol* search4Insert(char* name, enum NameSrc ns) {
#ifdef SMTC_DEBUG
    fprintf(stderr, "[SEM DEGUB] Search for Insert: name: %s, ns: %d\n", name, ns);
    //printSymbolStack();
#endif
    int idx = hash_pjw(name);
    Symbol* p = symbolTable[idx];
    if(ns == NS_FUNC) {    
        while(p) {
            if(strcmp(name, p->name) == 0 && p->nameSrc == NS_FUNC) 
                return p;
            p = p->next;
        }
    }
    else if(ns == NS_STRUCT) {    
        while(p) {
            if(strcmp(name, p->name) == 0 && (p->nameSrc == NS_STRUCT || p->nameSrc == NS_GVAR || p->nameSrc == NS_LVAR))
                return p;
            p = p->next;
        }
    }
    else if(ns == NS_GVAR) {    
        while(p) {
            if(strcmp(name, p->name) == 0 && (p->nameSrc == NS_STRUCT || p->nameSrc == NS_GVAR))   
                return p;
            p = p->next;
        }
        p = symbolStack[0];
        while(p) {
            if(p->nameSrc == NS_STRUCT) {
                Field* cField = p->dataType->structure.fieldList;
                while(cField) {
                    if(strcmp(name, cField->name) == 0)
                        return fieldSymbol;
                    cField = cField->next;
                }
            }
            p = p->stackNext;
        }
    }
    else if(ns == NS_LVAR) {
        p = symbolStack[stackTop];
        while(p) {
            if(p->nameSrc == NS_LVAR && strcmp(name, p->name) == 0)
                return p;
            p = p->stackNext;
        }
        p = symbolTable[idx];
        while(p) {
            if(strcmp(name, p->name) == 0 && p->nameSrc == NS_STRUCT)
                return p;
            p = p->next;
        }
    }
    return NULL;
}

Symbol* search4Field(char* name) {
#ifdef SMTC_DEBUG
    fprintf(stderr, "[SEM DEGUB] Search for Insert: name: %s, ns: Field\n", name);
    //printSymbolStack();
#endif
    int idx = hash_pjw(name);
    Symbol* p = symbolTable[idx];
    while(p) {
        if(p->nameSrc == NS_GVAR) 
            return p;
        p = p->next;
    }
    p = symbolStack[0];
    while(p) {
        if(p->nameSrc == NS_STRUCT) {
            Field* cField = p->dataType->structure.fieldList;
            while(cField) {
                if(strcmp(name, cField->name) == 0)
                    return fieldSymbol;
                cField = cField->next;
            }
        }
        p = p->stackNext;
    }
    return NULL;
}

void insertSymbol(Symbol* newSymbol) {
    if(newSymbol == NULL)
        return;
    int idx = hash_pjw(newSymbol->name);
    switch(newSymbol->nameSrc) {
    case NS_LVAR:
        newSymbol->next = symbolTable[idx];
        newSymbol->stackNext = symbolStack[stackTop];
        symbolTable[idx] = newSymbol;
        symbolStack[stackTop] = newSymbol;
        break;
    default:
        newSymbol->next = symbolTable[idx];
        newSymbol->stackNext = symbolStack[0];
        symbolTable[idx] = newSymbol;
        symbolStack[0] = newSymbol;
        break;
    }
#ifdef SMTC_DEBUG
    fprintf(stderr, "[SEM DEGUB] Insert Symbol: name: %s\n", newSymbol->name);
    printSymbolStack();
#endif
}

Symbol* search4Use(char* name, enum NameSrc ns) {
#ifdef SMTC_DEBUG
    fprintf(stderr, "[SEM DEGUB] Search for Use: name: %s, ns: %d\n", name, ns);
    //printSymbolStack();
#endif
    int tableIndex = hash_pjw(name);
    Symbol* p = NULL;
    /* Lab2
    if(ns == NS_FUNC || ns == NS_STRUCT || ns == NS_GVAR) {
        p = symbolTable[tableIndex];
        while(p) {
            if(p->nameSrc == ns) 
                return p;
            p = p->next;
        }
    }
    else if(ns == NS_LVAR) {
        int stackIndex = stackTop;
        while(stackIndex >= 1) {
            p = symbolStack[stackIndex];
            while(p) {
                if(p->nameSrc == NS_LVAR && strcmp(name, p->name) == 0)
                    return p;
                p = p->stackNext;
            }
            stackIndex--;
        }
    }*/
    /* Lab3 */
    p = symbolTable[tableIndex];
    while(p) {
        if(p->nameSrc == ns && strcmp(name, p->name) == 0) 
            return p;
        p = p->next;
    }
    return NULL;
}

void stackPop() {
    if(stackTop <= 0 || stackTop >= stackSize)
        return;
    Symbol* head = symbolStack[stackTop];
    Symbol* p = NULL;
    Symbol* tmp = NULL;
    int idx;
    while(head) {
        // table
        /* Lab2
        idx = hash_pjw(head->name);
        p = symbolTable[idx];
        if(p == head) {
            symbolTable[idx] = p->next;
        }
        else {
            while(p) {
                if(p->next && p->next == head) {
                    p->next = p->next->next;
                    break;
                }
                p = p->next;
            }
        }*/
        // stack
        tmp = head->stackNext;
        head->stackNext = NULL;
        head = tmp;
    }
    symbolStack[stackTop] = NULL;
    stackTop--;
#ifdef SMTC_DEBUG
    fprintf(stderr, "[SEM DEGUB] Clear Stack Top\n");
    printSymbolStack();
#endif
}

void stackPush() {
    // handle stack error
    // assert(stackTop >= 0);
    if(stackTop == stackSize-1) {
        stackSize *= 2;
        StackEle* newStack = (StackEle*)malloc(sizeof(StackEle)*stackSize);
        memset(newStack, 0, stackSize * sizeof(StackEle));
        int i=0;
        while(i<=stackTop) {
            newStack[i] = symbolStack[i];
            i++;
        }
        free(symbolStack);
        symbolStack = newStack;
    }
    stackTop++;
}

void printSymbolStack() {
    int idx = stackTop;
    Symbol* p;
    fprintf(stderr, "------------------------------\n");
    while(idx >= 0) {
        p = symbolStack[idx];
        fprintf(stderr, "stack %d:\n", idx);
        while(p) {
            switch(p->nameSrc) {
            case NS_FUNC:
                fprintf(stderr, "\tname: %s, src: %s\n", p->name, "FUNC");
                break;
            case NS_STRUCT:
                fprintf(stderr, "\tname: %s, src: %s\n", p->name, "STRUCT");
                break;
            case NS_GVAR:
                fprintf(stderr, "\tname: %s, src: %s\n", p->name, "GVAR");
                break;
            case NS_LVAR:
                fprintf(stderr, "\tname: %s, src: %s\n", p->name, "LVAR");
                break;
            default:
                fprintf(stderr, "\tname: %s\n", p->name);
                break;
            }
            p = p->stackNext;
        }
        idx--;
    }
    fprintf(stderr, "------------------------------\n");
}

void printSymbolTable() {
    fprintf(stderr, "-------- Symbol Table -------\n");
    for(int i=0;i<MAX_ST_SIZE;i++) {
        if(!symbolTable[i]) continue;
        fprintf(stderr, "index %d: \n", i);
        Symbol* p = symbolTable[i];
        while(p) {
            fprintf(stderr, "%s | ", p->name);
            p = p->next;
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "------------------------------\n");
}

// Lab3
int getsizeof(DataType* type) {
    if(type == NULL)
        return 0;
    int size = 0;
    switch (type->kind) {
    case DT_BASIC:
        size = 4;
        break;
    case DT_ARRAY:
        size = type->array.size * getsizeof(type->array.elem);
        break;
    case DT_STRUCT: {
        Field* curField = type->structure.fieldList;
        while(curField) {
            size += getsizeof(curField->dataType);
            curField = curField->next;
        }
        break;
    }
    default:
        break;
    }
    return size;
}

int getFieldOffset(DataType* specifier, char* field, DataType** fieldType) {
    assert(specifier->kind == DT_STRUCT);
    int offset = 0;
    Field* curField = specifier->structure.fieldList;
    while(curField) {
        if(strcmp(curField->name, field) == 0) {
            if(fieldType) *fieldType = curField->dataType;
            break;
        }
        offset += getsizeof(curField->dataType);
        curField = curField->next;
    }
    return offset;
}