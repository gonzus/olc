#include <stdio.h>
#include "olc.h"

int main()
{
    char code[256];
    int len;
    OLC_LatLon location;

#if 1
    // Encodes latitude and longitude into a Plus+Code.
    location.lat = 47.0000625;
    location.lon =  8.0000625;
    len = EncodeDefault(&location, code, 256);
    printf("%s (%d)\n", code, len);
    // => "8FVC2222+22"

    // Encodes latitude and longitude into a Plus+Code with a preferred length.
    len = Encode(&location, 16, code, 256);
    printf("%s (%d)\n", code, len);
    // => "8FVC2222+22GCCCCC"

    // Decodes a Plus+Code back into coordinates.
    OLC_CodeArea code_area;
    Decode(code, &code_area);
    printf("Code length: %14.10f : %14.10f to %14.10f : %14.10f (%lu)\n",
           code_area.lo.lat,
           code_area.lo.lon,
           code_area.hi.lat,
           code_area.hi.lon,
           code_area.len);
    // => 47.000062496 8.00006250000001 47.000062504 8.0000625305176 16

    int is_valid = IsValid(code);
    printf("Is Valid: %d\n", is_valid);
    // => true

    int is_full = IsFull(code);
    printf("Is Full: %d\n", is_full);
    // => true

    int is_short = IsShort(code);
    printf("Is Short: %d\n", is_short);
    // => true
#endif

    // Shorten a Plus+Codes if possible by the given reference latitude and
    // longitude.
    location.lat = 51.3708675;
    location.lon = -1.217765625;
    len = Shorten("9C3W9QCJ+2VX", &location, code, 256);
    printf("Shortened: %s\n", code);
    // => "CJ+2VX"

    // Extends a Plus+Code by the given reference latitude and longitude.
    RecoverNearest("CJ+2VX", &location, code, 256);
    printf("Recovered: %s\n", code);
    // => "9C3W9QCJ+2VX"

    return 0;
}
