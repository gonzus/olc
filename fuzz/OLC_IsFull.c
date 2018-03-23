#include <stdint.h>
#include <stddef.h>
#include "olc.h"

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    OLC_IsFull((const char*) Data, Size);
    return 0;
}
