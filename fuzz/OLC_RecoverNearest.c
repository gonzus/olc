#include <stdint.h>
#include <stddef.h>
#include "olc.h"

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    char buf[256];
    OLC_LatLon reference = { 0, 0 };
    OLC_RecoverNearest((const char*) Data, Size, &reference, buf, 256);
    return 0;
}
