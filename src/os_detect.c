#include <stdio.h>
#include <string.h>
#include "../include/os_detect.h"

int detect_os (void){

    FILE *file = fopen("/etc/os-release","r");

    if (file == NULL){
        return OS_UNKNOWN;
    } 

    char line[256];
    int os_type;

    while (fgets(line , sizeof(line) , file) != NULL){

        //arch familly ( arch,cachyos) "pacman"
        if (strstr(line,"arch") !=NULL){
            os_type = OS_ARCH;
            break;
        }

        //debian familly (debian,kali,ubuntu,mint) "apt"
        if (strstr(line, "debian") != NULL || strstr(line, "ubuntu") != NULL){
            os_type = OS_DEBIAN;
            break;
        }

        if (strstr(line, "fedora") != NULL || strstr(line, "rhel") != NULL){
            os_type = OS_FEDORA;
            break;
        }

        


    }

    fclose(file);
    return os_type;

}