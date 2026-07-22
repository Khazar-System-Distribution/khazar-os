#ifndef AI_SDK_ERRORS_H
#define AI_SDK_ERRORS_H

#include "common.h"

#define RETURN_IF_NULL(ptr) \
    do { if (!(ptr)) return -1; } while (0)

#define RETURN_VAL_IF_NULL(ptr, val) \
    do { if (!(ptr)) return (val); } while (0)

#define GOTO_IF_NULL(ptr, label) \
    do { if (!(ptr)) goto label; } while (0)

#endif
