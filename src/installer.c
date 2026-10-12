#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/cJSON.h"
#include "../include/os_detect.h"
#include "../include/installer.h"

static char* read_file_to_string(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
       
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char*) malloc(length + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, length, file);
    buffer[length] = '\0';

    fclose(file);
    return buffer;
}

int load_packages_from_json(int os_type, Package *app_list, int max_apps) {
    char *json_data = read_file_to_string("data/packages.json");
    if (!json_data) {
        return 0;
    }

    cJSON *json_root = cJSON_Parse(json_data);
    free(json_data);

    if (!json_root) {
       
        return 0;
    }

    const char *pkg_key = NULL;
    if (os_type == OS_ARCH) pkg_key = "pacman";
    else if (os_type == OS_DEBIAN) pkg_key = "apt";
    else if (os_type == OS_FEDORA) pkg_key = "dnf";
    else {
        cJSON_Delete(json_root);
        return 0;
    }

    int count = 0;
    cJSON *app_item = NULL;

    cJSON_ArrayForEach(app_item, json_root) {
        if (count >= max_apps) break;

        cJSON *name = cJSON_GetObjectItemCaseSensitive(app_item, "name");
        cJSON *category = cJSON_GetObjectItemCaseSensitive(app_item, "category");
        cJSON *packages = cJSON_GetObjectItemCaseSensitive(app_item, "packages");

        if (cJSON_IsString(name) && cJSON_IsString(category) && cJSON_IsObject(packages)) {
            cJSON *pkg_name = cJSON_GetObjectItemCaseSensitive(packages, pkg_key);
            
            if (cJSON_IsString(pkg_name) && (pkg_name->valuestring != NULL)) {
                strncpy(app_list[count].name, name->valuestring, sizeof(app_list[count].name));
                strncpy(app_list[count].category, category->valuestring, sizeof(app_list[count].category));
                strncpy(app_list[count].package_name, pkg_name->valuestring, sizeof(app_list[count].package_name));
                
                count++;
            }
        }
    }

    cJSON_Delete(json_root);

    return count;
}

int install_packages(int os_type, const char *packages) {
    if (packages == NULL || strlen(packages) == 0) {
       
        return -1;
    }

    char command[1024] = "";

    switch (os_type) {
        case OS_ARCH:
            snprintf(command, sizeof(command), "pkexec pacman -S --noconfirm %s", packages);
            break;
        case OS_DEBIAN:
            snprintf(command, sizeof(command), "pkexec apt-get install -y %s", packages);
            break;
        case OS_FEDORA:
            snprintf(command, sizeof(command), "pkexec dnf install -y %s", packages);
            break;
        default:
           
            return -1;
    }

    

    int result = system(command);

    return result;
}