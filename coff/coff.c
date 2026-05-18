#include "coff.h"
#include <string.h>

const char* coff_get_name(const CoffSymbol* sym, const char* strtab,
                          u32 strtab_size) {
    if(!sym->name.long_name.zeroes) {
        u32 off = sym->name.long_name.offset;
        if(off < strtab_size) return strtab + off;
        return NULL;
    }

    // short_name may not be null-terminated if exactly 8 chars
    static char short_name_buf[COFF_SYMBOL_SHORT_NAME_LEN + 1];
    memcpy(short_name_buf, sym->name.short_name, COFF_SYMBOL_SHORT_NAME_LEN);
    short_name_buf[COFF_SYMBOL_SHORT_NAME_LEN] = 0;
    return short_name_buf;
}
