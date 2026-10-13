#ifndef INSTALLER_H
#define INSTALLER_H

#define PKG_NAME_LEN     128
#define PKG_CATEGORY_LEN 50
#define PKG_PACKAGE_LEN  128

typedef struct {
    char name[PKG_NAME_LEN];
    char category[PKG_CATEGORY_LEN];
    char package_name[PKG_PACKAGE_LEN];   /* may hold several space-separated packages */
} Package;

/* Loads the apps available for os_type from packages.json.
 * Returns the number of entries written to app_list (0 on error). */
int load_packages_from_json(int os_type, Package *app_list, int max_apps);

/* Installs the space-separated package list through pkexec.
 * Returns the exit status of the package manager, or -1 on error. */
int install_packages(int os_type, const char *packages);

#endif
