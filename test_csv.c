#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "olc.h"

#define TEST_SINGLE 1

#define BASE_PATH "test_data"

typedef int (TestFunc)(const char* line, char* cp[], int cn);

static int process_file(const char* file, TestFunc func);
static int test_encoding(const char* line, char* cp[], int cn);
#if !defined(TEST_SINGLE) || TEST_SINGLE <= 0
static int test_short_code(const char* line, char* cp[], int cn);
static int test_validity(const char* line, char* cp[], int cn);
#endif

int main()
{
    struct Data {
        const char* file;
        TestFunc* func;
    } data[] = {
        { "encodingTests.csv", test_encoding },
#if !defined(TEST_SINGLE) || TEST_SINGLE <= 0
        { "shortCodeTests.csv", test_short_code },
        { "validityTests.csv", test_validity },
#endif
    };
    for (int j = 0; j < sizeof(data) / sizeof(data[0]); ++j) {
        int records = process_file(data[j].file, data[j].func);
        printf("%s => %d records\n", data[j].file, records);
    }

    return 0;
}

static int process_file(const char* file, TestFunc func)
{
    char full[1024];
    sprintf(full, "%s/%s", BASE_PATH, file);
    FILE* fp = fopen(full, "r");
    if (!fp) {
        printf("Could not open [%s]\n", full);
        return 0;
    }
    int count = 0;
    printf("============ %s ============\n", file);
    while (1) {
        char line[1024];
        if (!fgets(line, 1024, fp)) {
            break;
        }
        int blanks = 1;
        char* cp[100];
        int cn = 0;
        int first = -1;
        for (int j = 0; line[j] != '\0'; ++j) {
            if (isspace(line[j]) && blanks) {
                continue;
            }
            if (line[j] == '#' && blanks) {
                break;
            }
            blanks = 0;
            if (first < 0) {
                first = j;
            }
            if (line[j] == ',') {
                line[j] = '\0';
                cp[cn++] = line + first;
                first = j + 1;
                continue;
            }
            if (line[j] == '\n') {
                line[j] = '\0';
                cp[cn++] = line + first;
                first = -1;
                break;
            }
        }
        if (cn <= 0) {
            continue;
        }
        func(line, cp, cn);
        ++count;
    }
    fclose(fp);
    return count;
}

static int test_encoding(const char* line, char* cp[], int cn)
{
    if (cn != 7) {
        printf("test_encoding needs 7 columns per row, not %d\n", cn);
        return 0;
    }

    // code,lat,lng,latLo,lngLo,latHi,lngHi
    int ok = 0;

    char* code = cp[0];
#if !defined(TEST_SINGLE) || TEST_SINGLE <= 0
#else
    if (strcmp(code, "7FG49Q00+") != 0) {
        return 0;
    }
#endif
    int len = CodeLength(code);

    OLC_LatLon data_pos;
    data_pos.lat = strtod(cp[1], 0);
    data_pos.lon = strtod(cp[2], 0);

    // Encode the test location and make sure we get the expected code.
    char encoded[256];
    Encode(&data_pos, len, encoded, 256);
    ok = strcmp(code, encoded) == 0;
    printf("%-3.3s CODE [%s] [%s]\n", ok ? "OK" : "BAD", code, encoded);

    // Now decode the code and check we get the correct coordinates.
    OLC_CodeArea data_area;
    data_area.lat_lo = strtod(cp[3], 0);
    data_area.lon_lo = strtod(cp[4], 0);
    data_area.lat_hi = strtod(cp[5], 0);
    data_area.lon_hi = strtod(cp[6], 0);
    data_area.code_len = len;

    OLC_LatLon data_center;
    OLC_CodeArea_GetCenter(&data_area, &data_center);

    OLC_CodeArea decoded_area;
    Decode(code, &decoded_area);

    OLC_LatLon decoded_center;
    OLC_CodeArea_GetCenter(&decoded_area, &decoded_center);

    ok = fabs(data_center.lat - decoded_center.lat) < 1e-10;
    printf("%-3.3s LAT [%f:%f]\n", ok ? "OK" : "BAD", data_center.lat, decoded_center.lat);
    ok = fabs(data_center.lon - decoded_center.lon) < 1e-10;
    printf("%-3.3s LON [%f:%f]\n", ok ? "OK" : "BAD", data_center.lon, decoded_center.lon);

    return 0;
}

#if !defined(TEST_SINGLE) || TEST_SINGLE <= 0
static int test_short_code(const char* line, char* cp[], int cn)
{
    if (cn != 5) {
        printf("test_short_code needs 5 columns per row, not %d\n", cn);
        return 0;
    }

    // full code,lat,lng,shortcode,test_type
    // test_type is R for recovery only, S for shorten only, or B for both.
    int ok = 0;
    char code[256];
    char* full_code = cp[0];
    char* short_code = cp[3];
    char* type = cp[4];

    OLC_LatLon reference;
    reference.lat = strtod(cp[1], 0);
    reference.lon = strtod(cp[2], 0);

    // Shorten the code using the reference location and check.
    if (strcmp(type, "B") == 0 || strcmp(type, "S") == 0) {
        Shorten(full_code, &reference, code, 256);
        ok = strcmp(short_code, code) == 0;
        printf("%-3.3s SHORTEN [%s]: [%s] [%s]\n", ok ? "OK" : "BAD", full_code, short_code, code);
    }

    // Now extend the code using the reference location and check.
    if (strcmp(type, "B") == 0 || strcmp(type, "R") == 0) {
        RecoverNearest(short_code, &reference, code, 256);
        ok = strcmp(full_code, code) == 0;
        printf("%-3.3s RECOVER [%s]: [%s] [%s]\n", ok ? "OK" : "BAD", short_code, full_code, code);
    }

    return 0;
}

static int to_boolean(const char* s)
{
    if (!s || s[0] == '\0') {
        return 0;
    }
    if (strcasecmp(s, "false") == 0 ||
        strcasecmp(s, "no") == 0 ||
        strcasecmp(s, "f") == 0 ||
        strcasecmp(s, ".f.") == 0 ||
        strcasecmp(s, "n") == 0) {
        return 0;
    }
    return 1;
}

static int test_validity(const char* line, char* cp[], int cn)
{
    if (cn != 4) {
        printf("test_validity needs 4 columns per row, not %d\n", cn);
        return 0;
    }

    // code,isValid,isShort,isFull
    int ok = 0;
    char* code = cp[0];
    int is_valid = to_boolean(cp[1]);
    int is_short = to_boolean(cp[2]);
    int is_full = to_boolean(cp[3]);

    ok = IsValid(code) == is_valid;
    printf("%-3.3s IsValid [%s]: [%d]\n", ok ? "OK" : "BAD", code, is_valid);

    ok = IsFull(code) == is_full;
    printf("%-3.3s IsFull [%s]: [%d]\n", ok ? "OK" : "BAD", code, is_full);

    ok = IsShort(code) == is_short;
    printf("%-3.3s IsShort [%s]: [%d]\n", ok ? "OK" : "BAD", code, is_short);

    return 0;
}
#endif
