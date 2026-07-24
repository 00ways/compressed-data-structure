#include <00ways/cds.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
static enum {
    STATE_NULL = 0,
    STATE_READ = 1,
    STATE_WRITE = 2
} state = STATE_NULL;
static const char* filename = NULL;
static int parseArgs(int argc, char* const argv[]) {
    if (argc < 2) return 1;
    for (int argi = 1; argi < argc; argi++) {
        const char* argo = argv[argi];
        if (strcmp(argo, "--write") == 0 && state == STATE_NULL)
            state = STATE_WRITE;
        else if (strcmp(argo, "--read") == 0 && state == STATE_NULL)
            state = STATE_READ;
        else if (filename == NULL) {
            filename = argo;
        }
        else {
            printf("%s\n", filename);
            return 1;
        }
    }
    if (state == STATE_NULL) state = STATE_READ;
    if (filename == NULL) {
        return 1;
    }
    printf("%d: %s\n", state, filename);
    return 0;
}
int read() {
    printf("reading\n");
    cdsmap* map = cdsMapFile(filename);
    if (map == NULL) {
        perror("File not found");
        return 1;
    }
    printf("%s\n", cdsMapToString(map));
    char* line = NULL;
    char *linetr = NULL;
    size_t linesz = 0;
    while (true) {
        printf("> ");
        if (getline(&line, &linesz, stdin) == -1) continue;
        
        char* buffer = calloc(strlen(line) + 1, sizeof(char)), *ptr = buffer;
        if (buffer == NULL) {
            printf("Failed to create buffer\n");
            continue;
        }
        for (int i = 0; i < strlen(line); i++) {
            if (line[i] == '\n') continue;
            if (line[i] == '\t') continue;
            *ptr ++ = line[i];
        }
        *ptr = 0x00;
        if (strcmp(buffer, "*exit") == 0) {
            break;
        }
        if (strlen(buffer) == 0) {
            continue;
        }
        printf(">> %s\n", buffer);
        char* file = cdsMapReadFile(map, buffer);
        if (file != NULL) {
            printf("%s\n", file);
            free(file);
        }
        free(buffer);
    }
    cdsCloseMap(map);
    return 0;
}
int write() {
    printf("writing\n");
    printf("Select files to write:\n");
    char* line = NULL;
    char *linetr = NULL;
    size_t linesz = 0;
    int filecount = 0;
    char** files = calloc(1, sizeof(char*));
    while (true) {
        printf("filecount: %d\n", filecount);
        printf("> ");
        if (getline(&line, &linesz, stdin) == -1) continue;
        
        char* buffer = calloc(16, sizeof(char)), *ptr = buffer;
        for (int i = 0; i < 16; i++) {
            if (line[i] == '\n') continue;
            if (line[i] == '\t') continue;
            *ptr ++ = line[i];
        }
        if (strcmp(buffer, "*write") == 0) {
            break;
        }
        if (strlen(buffer) == 0) {
            continue;
        }
        printf(">> %s\n", buffer);
        files[filecount] = calloc(strlen(buffer), sizeof(char));
        memcpy(files[filecount], buffer, strlen(buffer));
        filecount++;
        free(buffer);
    }
    cdsmap* map = cdsMapCreate(filename, filecount);
    FILE* streamtmp = tmpfile();
    for (int i = 0; i < filecount; i++) {
        FILE* reader = fopen(files[i], "rb");
        fseek(reader, 0, SEEK_END);
        long length = ftell(reader);
        fseek(reader, 0, SEEK_SET);
        printf("%s length: %ld\n", files[i], length);
        char* buffer = calloc(length, sizeof(char));
        fread(buffer, sizeof(char), length, reader);
        fclose(reader);
        printf("%s\n", buffer);
        fwrite(buffer, sizeof(char), length, streamtmp);
        free(buffer);
        cdsMapAddRecord(map, files[i], length);
    }
    FILE* target = NULL;
    cdsMapWriteFileHeader(map, &target);
    if (target == NULL) {
        fclose(streamtmp);
        return 1;
    }
    fseek(streamtmp, 0, SEEK_END);
    long length = ftell(streamtmp);
    printf(".tmp length: %ld\n", length);
    fseek(streamtmp, 0, SEEK_SET);
    rewind(streamtmp);
    char* buffer = calloc(length, sizeof(char));
    fread(buffer, sizeof(char) * length, 1, streamtmp);
    printf("copying temp: %s\n", buffer);
    fwrite(buffer, sizeof(char) * length, 1, target);
    fclose(target);
    fclose(streamtmp);
    return 0;
}

int main(int argc, char* const argv[]) {
    if (parseArgs(argc, argv) != 0) {
        printf("Error\n");
        return 1;
    }
    switch (state) {
    case STATE_READ: return read();
    case STATE_WRITE: return write();
    default: return 1;
    }
}