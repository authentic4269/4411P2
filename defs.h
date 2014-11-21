#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

/* if debuging is desired set value to 1 */
#define DEBUG 1
#define INTERRUPT_DEFER 0
#define INTERRUPT_DROP 1

/* for now kernel printfs are just regular printfs */
#define kprintf printf

#define GetLastError() errno

#define AbortOnCondition(cond,message)                       \
    if (cond) {                                              \
        printf("Abort: %s:%d %d, MSG:%s\n",                  \
               __FILE__, __LINE__, GetLastError(), message); \
        exit(1);                                             \
    }

#define AbortOnError(fctcall)                         \
    if (fctcall == 0) {                               \
        printf("Error: file %s line %d: code %d.\n", \
               __FILE__, __LINE__, GetLastError());   \
        exit(1);                                      \
    }


#endif /* _CONFIG_H_ */
