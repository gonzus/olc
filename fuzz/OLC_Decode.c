#include <stdint.h>
#include <stddef.h>
#include "olc.h"

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  OLC_CodeArea area;
  OLC_Decode((const char*)Data, Size, &area);
  return 0;
}
