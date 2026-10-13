#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "../include/cJSON.h"
#include "../include/os_detect.h"
#include "../include/installer.h"

static char *read_file_to_string(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return NULL; }
    long length = ftell(file);
    if (length < 0) { fclose(file); return NULL; }
    rewind(file);

    char *buffer = (char *) malloc((size_t) length + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    size_t read_bytes = fread(buffer, 1, (size_t) length, file);
    fclose(file);

    buffer[read_bytes] = '\0';
    return buffer;
}

/* packages.json is looked up in the current directory first, then next to the
 * executable, so the program also works when launched from somewhere else. */
static char *read_packages_file(void) {
    char *data = read_file_to_string("data/packages.json");
    if (data) return data;

    char exe[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (n > 0) {
        exe[n] = '\0';
        char path[PATH_MAX + 32];
        snprintf(path, sizeof(path), "%s/data/packages.json", dirname(exe));
        data = read_file_to_string(path);
    }
    return data;
}

static void copy_str(char *dst, size_t size, const char *src) {
    snprintf(dst, size, "%s", src);   /* always NUL-terminated, unlike strncpy */
}

int load_packages_from_json(int os_type, Package *app_list, int max_apps) {
    const char *pkg_key = NULL;
    if (os_type == OS_ARCH) pkg_key = "pacman";
    else if (os_type == OS_DEBIAN) pkg_key = "apt";
    else if (os_type == OS_FEDORA) pkg_key = "dnf";
    else return 0;

    char *json_data = read_packages_file();
    if (!json_data) {
        return 0;
    }

    cJSON *json_root = cJSON_Parse(json_data);
    free(json_data);

    if (!json_root) {
        return 0;
    }
    if (!cJSON_IsArray(json_root)) {
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

            if (cJSON_IsString(pkg_name) && pkg_name->valuestring != NULL) {
                copy_str(app_list[count].name, sizeof(app_list[count].name), name->valuestring);
                copy_str(app_list[count].category, sizeof(app_list[count].category), category->valuestring);
                copy_str(app_list[count].package_name, sizeof(app_list[count].package_name), pkg_name->valuestring);
                count++;
            }
        }
    }

    cJSON_Delete(json_root);
    return count;
}

/* Package names may only contain these characters; anything else is rejected,
 * so nothing coming from a JSON file can ever be interpreted by a shell. */
static int is_valid_package_list(const char *s) {
    for (; *s; s++) {
        char c = *s;
        int ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                 (c >= '0' && c <= '9') || c == '+' || c == '.' ||
                 c == '_' || c == '-' || c == ' ';
        if (!ok) return 0;
    }
    return 1;
}

int install_packages(int os_type, const char *packages) {
    if (packages == NULL || packages[0] == '\0' || !is_valid_package_list(packages)) {
        return -1;
    }

    const char *prefix[4];
    switch (os_type) {
        case OS_ARCH:   prefix[0] = "pacman";  prefix[1] = "-S";      prefix[2] = "--noconfirm"; prefix[3] = "--needed"; break;
        case OS_DEBIAN: prefix[0] = "apt-get"; prefix[1] = "install"; prefix[2] = "-y";          prefix[3] = NULL;       break;
        case OS_FEDORA: prefix[0] = "dnf";     prefix[1] = "install"; prefix[2] = "-y";          prefix[3] = NULL;       break;
        default: return -1;
    }

    char *copy = strdup(packages);
    if (!copy) return -1;

    /* argv: pkexec <manager> <opts...> <pkg...> NULL */
    size_t max_args = 6 + strlen(copy) / 2 + 2;
    char **argv = calloc(max_args, sizeof(char *));
    if (!argv) { free(copy); return -1; }

    size_t n = 0;
    argv[n++] = "pkexec";
    for (int i = 0; i < 4; i++) {
        if (prefix[i]) argv[n++] = (char *) prefix[i];
    }
    size_t first_pkg = n;
    for (char *tok = strtok(copy, " "); tok != NULL; tok = strtok(NULL, " ")) {
        argv[n++] = tok;
    }
    argv[n] = NULL;

    if (n == first_pkg) { free(argv); free(copy); return -1; }

    int result = -1;
    pid_t pid = fork();
    if (pid == 0) {
        execvp("pkexec", argv);
        _exit(127);
    } else if (pid > 0) {
        int status = 0;
        if (waitpid(pid, &status, 0) == pid && WIFEXITED(status)) {
            result = WEXITSTATUS(status);
        }
    }

    free(argv);
    free(copy);
    return result;
}
