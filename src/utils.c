#include "utils.h"
#include <stdlib.h>
#include <stdio.h>

#define BUFFER_SIZE 4096

char *read_file_to_buffer(const char *filename){
    long filesize = get_file_size(filename);
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("error: fopen");
        return NULL;
    }

    char *buffer = malloc(filesize);
    if(fread(buffer, 1, filesize, file) != filesize) {
        perror("error: fread");
        return NULL; 
    }
    
    fclose(file);

    return buffer;
}

int write_buffer_to_file(const char *file_name, const char *buffer){
    return 0;
}

long get_file_size(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("error: fopen");
        return -1;
    }
    if(fseek(file, 0, SEEK_END) != 0) {
        perror("error: fseek");
        fclose(file);
        return -1;
    }

    long size = ftell(file);
    if (size == -1) perror("error: ftell");

    fclose(file);

    return size;
}