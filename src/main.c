#include <stdio.h>
#include "../include/os_detect.h"
#include "../include/ui.h"

int main(void) {
    int os = detect_os();
    
    if (os == OS_UNKNOWN) {
        return 1;
    }

    run_ui(os);

    return 0;
}