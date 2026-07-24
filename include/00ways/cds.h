#ifndef __00ways_cds_h
#define __00ways_cds_h

#include <stdint.h>
#include <stdio.h>
typedef struct cdsmap cdsmap;

cdsmap* cdsMapFile(const char* filename);
cdsmap* cdsMapCreate(const char* filename, int recordcount);
void cdsMapAddRecord(cdsmap* map, char* filename, uint64_t length);
void cdsCloseMap(cdsmap* map);

char* cdsMapToString(cdsmap* map);
char* cdsMapReadFile(cdsmap* map, char* filename);
int cdsMapWriteFileHeader(cdsmap* map, FILE** fptr);

#endif