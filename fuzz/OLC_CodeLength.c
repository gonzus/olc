#include <stdint.h>
#include <stddef.h>
#include "olc.h"

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    OLC_CodeLength((const char*) Data, Size);
    return 0;
}
