#include <stdio.h>
#include <string.h>
#include "../include/os_detect.h"

/* value looks like: ubuntu   or   "ubuntu debian"   or   'rhel fedora' */
static int match_family(const char *value) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", value);

    for (char *p = buf; *p; p++) {
        if (*p == '"' || *p == '\'' || *p == '\n' || *p == '\r') *p = ' ';
    }

    for (char *tok = strtok(buf, " \t"); tok != NULL; tok = strtok(NULL, " \t")) {
        if (strcmp(tok, "arch") == 0)                                  return OS_ARCH;
        if (strcmp(tok, "debian") == 0 || strcmp(tok, "ubuntu") == 0)  return OS_DEBIAN;
        if (strcmp(tok, "fedora") == 0 || strcmp(tok, "rhel") == 0)    return OS_FEDORA;
    }
    return OS_UNKNOWN;
}

int detect_os(void) {
    FILE *file = fopen("/etc/os-release", "r");
    if (file == NULL) {
        file = fopen("/usr/lib/os-release", "r");
    }
    if (file == NULL) {
        return OS_UNKNOWN;
    }

    char line[256];
    int os_type = OS_UNKNOWN;
    int from_id_like = OS_UNKNOWN;

    while (fgets(line, sizeof(line), file) != NULL) {
        if (strncmp(line, "ID=", 3) == 0) {
            /* ID has priority: arch, cachyos(ID_LIKE=arch), debian, ubuntu, fedora... */
            int r = match_family(line + 3);
            if (r != OS_UNKNOWN) { os_type = r; break; }
        } else if (strncmp(line, "ID_LIKE=", 8) == 0) {
            /* derivatives: cachyos, manjaro -> arch | kali, mint -> debian/ubuntu */
            int r = match_family(line + 8);
            if (r != OS_UNKNOWN) from_id_like = r;
        }
    }

    fclose(file);
    return os_type != OS_UNKNOWN ? os_type : from_id_like;
}
