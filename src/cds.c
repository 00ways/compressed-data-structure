#include <00ways/cds.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#ifdef _WIN32
/* This code is public domain -- Will Hartung 4/9/09 */
size_t getline(char **lineptr, size_t *n, FILE *stream) {
    char *bufptr = NULL;
    char *p = bufptr;
    size_t size;
    int c;

    if (lineptr == NULL) {
        return -1;
    }
    if (stream == NULL) {
        return -1;
    }
    if (n == NULL) {
        return -1;
    }
    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if (c == EOF) {
        return -1;
    }
    if (bufptr == NULL) {
        bufptr = malloc(128);
        if (bufptr == NULL) {
            return -1;
        }
        size = 128;
    }
    p = bufptr;
    while(c != EOF) {
        if ((p - bufptr) > (size - 1)) {
            size = size + 128;
            bufptr = realloc(bufptr, size);
            if (bufptr == NULL) {
                return -1;
            }
        }
        *p++ = c;
        if (c == '\n') {
            break;
        }
        c = fgetc(stream);
    }

    *p++ = '\0';
    *lineptr = bufptr;
    *n = size;

    return p - bufptr - 1;
}
#endif

#if !defined(max) || !defined(min)
#define max(a, b) (a > b ? a : b)
#define min(a, b) (a < b ? a : b)
#endif

static const char magicNumber[2] = {
    0xCD,
    0x50
};
static const char sizeMask = 0b1111;

enum cdsSize {
    BYTE_1 = 0b0001,
    BYTE_2 = 0b0010,
    BYTE_4 = 0b0100,
    BYTE_8 = 0b1000,
};
static int fwriteSized(FILE* stream, enum cdsSize size, uint64_t* data) {
    union resize{
        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;
    } d;
    d.u64 = *data;
    switch (size) {
    case BYTE_1: 
        printf("writing bytes: 1 (%02X)\n", d.u8);
        if (fwrite(&(d.u8), sizeof(uint8_t), 1, stream) < 1)
            return 1;
        return 0;
    case BYTE_2: 
        printf("writing bytes: 2(%02X)\n", d.u16);
        if (fwrite(&(d.u16), sizeof(uint16_t), 1, stream) < 1)
            return 1;
        return 0;
    case BYTE_4: 
        printf("writing bytes: 4(%02X)\n", d.u32);
        if (fwrite(&(d.u32), sizeof(uint32_t), 1, stream) < 1)
            return 1;
        return 0;
    case BYTE_8: 
    #ifdef _WIN32
        printf("writing bytes: 8(%02llX)\n", d.u64);
    #else
        printf("writing bytes: 8(%02lX)\n", d.u64);
    #endif
        if (fwrite(&(d.u64), sizeof(uint64_t), 1, stream) < 1)
            return 1;
        return 0;
    }
    return 0;
}
static int freadSized(uint64_t* data, enum cdsSize size, FILE* stream) {
    return fread(data, size, 1, stream);
}
static int skipSized(FILE* stream, enum cdsSize size) {
    switch (size) {
    case BYTE_1: fseek(stream, sizeof(uint8_t), SEEK_CUR); break;
    case BYTE_2: fseek(stream, sizeof(uint16_t), SEEK_CUR); break;
    case BYTE_4: fseek(stream, sizeof(uint32_t), SEEK_CUR); break;
    case BYTE_8: fseek(stream, sizeof(uint64_t), SEEK_CUR); break;
    }
    return 0;
}
struct cdsRecord {
    char* filename;
    uint64_t length;
    struct cdsRecord* next;
};
struct cdsmap {
    const char* filename;
    enum cdsSize size;
    uint64_t recordCount;
    uint64_t origin;
    struct cdsRecord* recordsStack;
};

static struct cdsRecord* readRecord(FILE* fptr, enum cdsSize size) {
    struct cdsRecord* record = malloc(sizeof(struct cdsRecord));
    char length = 0x00;
    fread(&length, sizeof(char), 1, fptr);
    record->filename = calloc(length + 1, sizeof(char));
    fread(record->filename, sizeof(char), length, fptr);
    record->filename[length] = 0x00;
    freadSized(&record->length, size, fptr);
    // printf("filename: %s\n", );
    record->next = NULL;
    return record;
}
cdsmap* cdsMapFile(const char* filename) {
    FILE* fptr = fopen(filename, "rb");
    if (fptr == NULL) {
        return NULL;
    }
    uint64_t recordCount = 0, origin = 0;
    char magic[2] = {0x00, 0x00};
    fread(magic, sizeof(char) * 2, 1, fptr);
    if (!(magic[0] == magicNumber[0] && (magic[1] & (~sizeMask)) == (magicNumber[1] & (~sizeMask)))) {
        perror("Signature not recognized\n");
        return NULL;
    }
    enum cdsSize size = magic[1] & sizeMask;
    
    freadSized(&recordCount , size,fptr);
    if (recordCount == 0) {
        perror("No records found");
        fclose(fptr);
        return NULL;
    }
    freadSized(&origin, size, fptr);
    int current = 0;
    struct cdsRecord* record = NULL;
    struct cdsRecord** cursor = &record;
    while(current++ < recordCount) {
        *cursor = readRecord(fptr, size);
        cursor = &(*cursor)->next;
        printf("read %d\n", current);
    }
    *cursor = NULL;
    cdsmap* map = malloc(sizeof(cdsmap));
    map->filename = filename;
    map->recordsStack = record;
    map->origin = origin;
    map->recordCount = recordCount;
    fclose(fptr);
    printf("map %s:\n", map->filename);
    printf("Origin: %ld\n", map->origin);
    printf("Records: %ld\n", map->recordCount);
    return map;
}
cdsmap* cdsMapCreate(const char* filename, int recordcount) {
    cdsmap* map = malloc(sizeof(struct cdsmap));
    map->filename = filename;
    map->recordCount = recordcount;
    map->recordsStack = NULL;
    return map;
}
void cdsMapAddRecord(cdsmap* map, char* filename, uint64_t length) {
    struct cdsRecord** current = &map->recordsStack;
    uint64_t offset = 0;
    while (*current != NULL) {
        current = &(*current)->next;
    }
    *current = malloc(sizeof(struct cdsRecord));
    (*current)->filename = calloc(strlen(filename), sizeof(char));
    memcpy((*current)->filename, filename, strlen(filename));
    (*current)->length = length;
    (*current)->next = NULL;
}
void cdsCloseMap(cdsmap* map) {
    if (!map) return;

    struct cdsRecord* cursor = map->recordsStack;
    while (cursor != NULL) {
        struct cdsRecord* next = cursor->next;
        free(cursor);
        cursor = next;
    }
    free(map);
}
char* cdsMapReadFile(cdsmap* map, char* filename) {
    struct cdsRecord* cursor = map->recordsStack;
    uint64_t offset = map->origin;
    uint64_t length = 0;
    while (cursor != NULL) {
        // sprintf(buffer, "%s\n%s %ld + %ld", buffer, cursor->filename, cursor->location, cursor->length);
        if (strcmp(cursor->filename, filename) == 0) {
            length = cursor->length;
            break;
        }
        else {
            offset += cursor->length;
        }
        cursor = cursor->next;
    }
    if (offset == 0 && length == 0) {
        return NULL;
    }
    FILE* fstream = fopen(map->filename, "rb");
    fseek(fstream, offset, SEEK_SET);
    char* buffer = calloc(length + 1, sizeof(char));
    fread(buffer, length, sizeof(char), fstream);
    buffer[length] = 0x00;
    return buffer;
}
static enum cdsSize analyzeRecords(struct cdsRecord* recordRoot) {
    enum cdsSize size = BYTE_1;
    uint64_t length = 0;
    for (struct cdsRecord* node = recordRoot; node; node = node->next) {
        length += node->length;
        if (node->length > UINT32_MAX) size = max(size, BYTE_8);
        else if (node->length > UINT16_MAX) size = max(size, BYTE_4);
        else if( node->length > UINT8_MAX) size = max(size, BYTE_2);
        else size = max(size, BYTE_1);
        if (length > UINT32_MAX) size = max(size, BYTE_8);
        else if (length > UINT16_MAX) size = max(size, BYTE_4);
        else if(length > UINT8_MAX) size = max(size, BYTE_2);
        else size = max(size, BYTE_1);
    }
    return size;
}


static int cdsMapWriteFileRecord(FILE* fptr, struct cdsRecord* record, enum cdsSize size) {
    char length = (char) (strlen(record->filename));
    if (fwrite(&length, sizeof(char), 1, fptr) < 1) return 1;
    if (fwrite(record->filename, sizeof(char), length, fptr) < length) return 1;
    if (fwriteSized(fptr, size, &record->length) != 1) return 1;
    return 0;
}
int cdsMapWriteFileHeader(cdsmap* map, FILE** pfptr) {
    FILE* fstream = fopen(map->filename, "wb");
    if (fstream == NULL) {
        printf("Unable to write file\n");
        return 1;
    }
    printf("[write] recordCount: %ld\n", map->recordCount);
    enum cdsSize size;
    if (map->recordCount > UINT32_MAX) size = BYTE_8;
    else if (map->recordCount > UINT16_MAX) size = BYTE_4;
    else if( map->recordCount > UINT8_MAX) size = BYTE_2;
    else size = BYTE_1;
    size = max(size, analyzeRecords(map->recordsStack));
    printf("size: %d\n", size);
    fseek(fstream, sizeof(char) * 2, SEEK_CUR);
    fwriteSized(fstream, size, &map->recordCount);
    skipSized(fstream, size);
    // fwrite(&map->origin, sizeof(uint64_t), 1, fstream);
    struct cdsRecord* current = map->recordsStack;
    while (current != NULL) {
        cdsMapWriteFileRecord(fstream, current, size);
        current = current->next;
    }
    long cursor = ftell(fstream);
    map->origin = cursor;
    fseek(fstream, /*magic offset */ 2, SEEK_SET);
    skipSized(fstream, size);
    printf("origin: %02lx (at %02lx)\n", map->origin, ftell(fstream));
    fwriteSized(fstream, size, &map->origin);
    fseek(fstream, 0, SEEK_SET);
    fwrite(magicNumber, sizeof(char), 1, fstream);
    char suffix = (magicNumber[1] & ~sizeMask) | (sizeMask & size);
    fwrite(&suffix, sizeof(char), 1, fstream);
    fseek(fstream, cursor, SEEK_SET);
    printf("cursor at initial position: %s\n", cursor == ftell(fstream) ? "TRUE" : "FALSE");
    printf("map->origin: %s\n", map->origin == ftell(fstream) ? "TRUE" : "FALSE");
    *pfptr = fstream;
    return 0;
}
char* cdsMapToString(cdsmap* map) {
    struct cdsRecord* cursor = map->recordsStack;
    char* buffer = calloc(map->recordCount * (1 + 16 + 1 + 8 + 3 + 8), sizeof(char));
    uint64_t offset = 0;
    while (cursor != NULL) {
        #ifdef _WIN32
        sprintf(buffer, "%s\n%s %I64d + %I64d", buffer, cursor->filename, offset, cursor->length);
        #else
        sprintf(buffer, "%s\n%s %ld + %ld", buffer, cursor->filename, offset, cursor->length);
        #endif
        offset += cursor->length;
        cursor = cursor->next;
    }
    return buffer;
}
