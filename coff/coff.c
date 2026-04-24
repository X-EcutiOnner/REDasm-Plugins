#include "coff.h"

const char* coff_get_name(const CoffSymbol* sym, const char* strtab,
                          u32 strtab_size) {
    if(!sym->name.long_name.zeroes) {
        u32 off = sym->name.long_name.offset;
        if(off < strtab_size) return strtab + off;
        return NULL;
    }

    // inline name: may not be null-terminated if exactly 8 chars
    return sym->name.short_name;
}
