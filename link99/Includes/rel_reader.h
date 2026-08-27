#ifndef REL_READER_H
#define REL_READER_H

#include <stdio.h>
#include "rel99_format.h"

typedef struct {
    FILE *file;
    WORD bits_left;
    WORD chunk;
    WORD failed;
    WORD item_number;
} REL_READER;

typedef struct {
    WORD item;
    WORD type;
    WORD field;
    char symbol[16];
} REL_RECORD;

void rel_reader_init(REL_READER *reader, FILE *file);
WORD rel_read(REL_READER *reader, REL_RECORD *record);
const char *rel_item_name(WORD item);

#endif
