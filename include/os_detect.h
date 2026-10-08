#ifndef OS_DETECT_H
#define OS_DETECT_H

#define OS_UNKNOWN 0
#define OS_ARCH    1
#define OS_DEBIAN  2
#define OS_FEDORA  3

int detect_os(void);

#endif