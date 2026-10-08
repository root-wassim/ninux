#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/cJSON.h"
#include "../include/os_detect.h"
#include "../include/installer.h"


static char* read_file_to_string(const char *filename){

    FILE *file = fopen(filename,"rb");

    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char*) malloc(length + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    
    

}