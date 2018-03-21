#include <ctype.h>
#include <float.h>
#include <math.h>
#include "olc.h"

static int sanitize(const char* code, char* sanitized, int maxlen,
                    int* first_sep, int* first_pad);
static int is_valid(const char* sanitized, int len);
static int is_short(const char* sanitized, int len, int first_sep);
static int is_full(const char* sanitized, int len, int first_sep);
static int decode(char* sanitized, int len, int first_sep, int first_pad, OLC_CodeArea* decoded);
static size_t code_length(int len, int first_sep, int first_pad);

static const char kSeparator = '+';
static const size_t kSeparatorPosition = 8;
static const size_t kMaximumDigitCount = 32;
static const char kPaddingCharacter = '0';
static const char kAlphabet[] = "23456789CFGHJMPQRVWX";
static const size_t kEncodingBase = 20;
static const size_t kPairCodeLength = 10;
static const size_t kGridCols = 4;
static const size_t kGridRows = kEncodingBase / kGridCols;

// Latitude bounds are -kLatMaxDegrees degrees and +kLatMaxDegrees
// degrees which we transpose to 0 and 180 degrees.
static const double kLatMaxDegrees = 90;

// Longitude bounds are -kLonMaxDegrees degrees and +kLonMaxDegrees
// degrees which we transpose to 0 and 360.
static const double kLonMaxDegrees = 180;

// These will be defined later, during runtime.
static size_t kInitialExponent = 0;
static double kGridSizeDegrees = 0.0;

static void init_constants(void)
{
    static int inited = 0;

    if (inited) {
        return;
    }
    inited = 1;

    // Work out the encoding base exponent necessary to represent 360 degrees.
    kInitialExponent = floor(log(360) / log(kEncodingBase));

    // Work out the enclosing resolution (in degrees) for the grid algorithm.
    kGridSizeDegrees = 1 / pow(kEncodingBase, kPairCodeLength / 2 - (kInitialExponent + 1));
}

// Raises a number to an exponent, handling negative exponents.
static double pow_neg(double base, double exponent)
{
    if (exponent == 0) {
        return 1;
    }

    if (exponent > 0) {
        return pow(base, exponent);
    }

    return 1 / pow(base, fabs(exponent));
}

// Compute the latitude precision value for a given code length. Lengths <= 10
// have the same precision for latitude and longitude, but lengths > 10 have
// different precisions due to the grid method having fewer columns than rows.
static double compute_precision_for_length(int code_length)
{
    if (code_length <= 10) {
        return pow_neg(kEncodingBase, floor((code_length / -2) + 2));
    }

    return pow_neg(kEncodingBase, -3) / pow(5, code_length - 10);
}

// Finds the position of a char in the encoding alphabet.
static int get_alphabet_position(char c)
{
    for (int j = 0; j < kEncodingBase; ++j) {
        if (c == kAlphabet[j]) {
            return j;
        }
    }
    return -1;
}

// Normalize a longitude into the range -180 to 180, not including 180.
static double normalize_longitude(double lon_degrees)
{
    while (lon_degrees < -kLonMaxDegrees) {
        lon_degrees += 360;
    }
    while (lon_degrees >= kLonMaxDegrees) {
        lon_degrees -= 360;
    }
    return lon_degrees;
}

// Adjusts 90 degree latitude to be lower so that a legal OLC code can be
// generated.
static double adjust_latitude(double lat_degrees, size_t code_length)
{
    if (lat_degrees < -90.0) {
        lat_degrees = -90.0;
    }
    if (lat_degrees >  90.0) {
        lat_degrees =  90.0;
    }
    if (lat_degrees < kLatMaxDegrees) {
        return lat_degrees;
    }
    // Subtract half the code precision to get the latitude into the code area.
    double precision = compute_precision_for_length(code_length);
    return lat_degrees - precision / 2;
}

// Encodes positive range lat,lng into a sequence of OLC lat/lng pairs.
// This uses pairs of characters (latitude and longitude in that order) to
// represent each step in a 20x20 grid. Each code, therefore, has 1/400th
// the area of the previous code.
int EncodePairs(double lat, double lng, size_t code_length, char* code, int maxlen)
{
    if ((code_length + 1) >= maxlen) {
        code[0] = '\0';
        return 0;
    }
    int pos = 0;
    // Provides the value of digits in this place in decimal degrees.
    init_constants();
    double resolution_degrees = pow(kEncodingBase, kInitialExponent);
    // Add two digits on each pass.
    for (size_t digit_count = 0;
         digit_count < code_length;
         digit_count += 2, resolution_degrees /= kEncodingBase) {
        size_t digit_value;

        // Do the latitude - gets the digit for this place and subtracts that
        // for the next digit.
        digit_value = floor(lat / resolution_degrees);
        lat -= digit_value * resolution_degrees;
        code[pos++] = kAlphabet[digit_value];

        // Do the longitude - gets the digit for this place and subtracts that
        // for the next digit.
        digit_value = floor(lng / resolution_degrees);
        lng -= digit_value * resolution_degrees;
        code[pos++] = kAlphabet[digit_value];

        // Should we add a separator here?
        if (pos == kSeparatorPosition && pos < code_length) {
            code[pos++] = kSeparator;
        }
    }
    while (pos < kSeparatorPosition) {
        code[pos++] = kPaddingCharacter;
    }
    if (pos == kSeparatorPosition) {
        code[pos++] = kSeparator;
    }
    code[pos] = '\0';
    return pos;
}

// Encodes a location using the grid refinement method into an OLC string.
// The grid refinement method divides the area into a grid of 4x5, and uses a
// single character to refine the area. The grid squares use the OLC characters
// in order to number the squares as follows:
//   R V W X
//   J M P Q
//   C F G H
//   6 7 8 9
//   2 3 4 5
// This allows default accuracy OLC codes to be refined with just a single
// character.
int EncodeGrid(double lat, double lng, size_t code_length, char* code, int maxlen)
{
    if ((code_length + 1) >= maxlen) {
        code[0] = '\0';
        return 0;
    }
    init_constants();
    int pos = 0;
    double lat_grid_size = kGridSizeDegrees;
    double lng_grid_size = kGridSizeDegrees;

    // To avoid problems with floating point, get rid of the degrees.
    lat = fmod(lat, 1);
    lng = fmod(lng, 1);
    lat = fmod(lat, lat_grid_size);
    lng = fmod(lng, lng_grid_size);
    for (size_t i = 0; i < code_length; i++) {
        // The following clause should never execute because of maximum code
        // length enforcement in other functions, but is here to prevent
        // division-by-zero crash from underflow.
        if ((lat_grid_size / kGridRows) <= DBL_MIN ||
            (lng_grid_size / kGridCols) <= DBL_MIN) {
            continue;
        }
        // Work out the row and column.
        size_t row = floor(lat / (lat_grid_size / kGridRows));
        size_t col = floor(lng / (lng_grid_size / kGridCols));
        lat_grid_size /= kGridRows;
        lng_grid_size /= kGridCols;
        lat -= row * lat_grid_size;
        lng -= col * lng_grid_size;
        code[pos++] = kAlphabet[row * kGridCols + col];
    }
    code[pos] = '\0';
    return pos;
}

int Encode(const OLC_LatLon* location, size_t code_length, char* code, int maxlen)
{
    int pos = 0;

    // Limit the maximum number of digits in the code.
    if (code_length > kMaximumDigitCount) {
        code_length = kMaximumDigitCount;
    }

    // Adjust latitude and longitude so they fall into positive ranges.
    double lat = adjust_latitude(location->lat, code_length) + kLatMaxDegrees;
    double lon = normalize_longitude(location->lon) + kLonMaxDegrees;
    size_t tmp = code_length;
    if (tmp > kPairCodeLength) {
        tmp = kPairCodeLength;
    }
    pos += EncodePairs(lat, lon, tmp, code + pos, maxlen - pos);
    // If the requested length indicates we want grid refined codes.
    if (code_length > kPairCodeLength) {
        pos += EncodeGrid(lat, lon, code_length - kPairCodeLength, code + pos, maxlen - pos);
    }
    code[pos] = '\0';
    return pos;
}

int EncodeDefault(const OLC_LatLon* location, char* code, int maxlen)
{
    return Encode(location, kPairCodeLength, code, maxlen);
}

int Decode(const char* code, OLC_CodeArea* decoded)
{
    char sanitized[256];
    int first_sep = 0;
    int first_pad = 0;
    int len = sanitize(code, sanitized, 256, &first_sep, &first_pad);
    if (len <= 0) {
        return 0;
    }

    return decode(sanitized, len, first_sep, first_pad, decoded);
}

int Shorten(const char* code, const OLC_LatLon* reference_location, char* shortened, int maxlen)
{
    char sanitized[256];
    int first_sep = 0;
    int first_pad = 0;
    int len = sanitize(code, sanitized, 256, &first_sep, &first_pad);
    if (len <= 0) {
        return 0;
    }

    if (!is_full(sanitized, len, first_sep)) {
        return 0;
    }
    if (first_pad > 0) {
        return 0;
    }

    OLC_CodeArea code_area;
    decode(sanitized, len, first_sep, first_pad, &code_area);

    OLC_LatLon center;
    OLC_CodeArea_GetCenter(&code_area, &center);

    // Ensure that latitude and longitude are valid.
    double lat = adjust_latitude(reference_location->lat, len);
    double lon = normalize_longitude(reference_location->lon);
    // How close are the latitude and longitude to the code center.
    double alat = fabs(center.lat - lat);
    double alon = fabs(center.lon - lon);
    double range = alat > alon ? alat : alon;

    int start = 0;
    const double safety_factor = 0.3;
    const int removal_lengths[3] = {8, 6, 4};
    for (int j = 0; j < sizeof(removal_lengths) / sizeof(removal_lengths[0]); ++j) {
        // Check if we're close enough to shorten. The range must be less than 1/2
        // the resolution to shorten at all, and we want to allow some safety, so
        // use 0.3 instead of 0.5 as a multiplier.
        int removal_length = removal_lengths[j];
        double area_edge = compute_precision_for_length(removal_length) * safety_factor;
        if (range < area_edge) {
            start = removal_length;
            break;
        }
    }
    int pos = 0;
    for (int j = start; code[j] != '\0'; ++j) {
        shortened[pos++] = code[j];
    }
    shortened[pos] = '\0';
    return pos;
}

int RecoverNearest(const char* short_code, const OLC_LatLon* reference_location, char* code, int maxlen)
{
    char sanitized[256];
    int first_sep = 0;
    int first_pad = 0;
    int len = sanitize(code, sanitized, 256, &first_sep, &first_pad);
    // printf("SANITIZED [%s] => [%s] %d %d %d\n", short_code, sanitized, len, first_sep, first_pad);
    if (len <= 0) {
        return 0;
    }
    len = code_length(len, first_sep, first_pad);

    if (!is_short(sanitized, len, first_sep)) {
        return 0;
    }

    // Ensure that latitude and longitude are valid.
    double lat = adjust_latitude(reference_location->lat, len);
    double lon = normalize_longitude(reference_location->lon);
    // Compute the number of digits we need to recover.
    size_t padding_length = kSeparatorPosition - first_sep;
    // The resolution (height and width) of the padded area in degrees.
    double resolution = pow_neg(kEncodingBase, 2.0 - (padding_length / 2.0));
    // Distance from the center to an edge (in degrees).
    double half_res = resolution / 2.0;
    // Use the reference location to pad the supplied short code and decode it.
    OLC_LatLon latlng = {lat, lon};
    char encoded[256];
    EncodeDefault(&latlng, encoded, 256);

    char new_code[1024];
    int pos = 0;
    for (int j = 0; j < padding_length && encoded[j] != '\0'; ++j) {
        new_code[pos++] = encoded[j];
    }
    for (int j = 0; short_code[j] != '\0'; ++j) {
        new_code[pos++] = short_code[j];
    }
    new_code[pos] = '\0';
    pos = sanitize(new_code, sanitized, 256, &first_sep, &first_pad);

    OLC_CodeArea code_rect;
    decode(sanitized, pos, first_sep, first_pad, &code_rect);

    OLC_LatLon center;
    OLC_CodeArea_GetCenter(&code_rect, &center);

    // How many degrees latitude is the code from the reference? If it is more
    // than half the resolution, we need to move it north or south but keep it
    // within -90 to 90 degrees.
    if (lat + half_res < center.lat && center.lat - resolution > -kLatMaxDegrees) {
        // If the proposed code is more than half a cell north of the reference location,
        // it's too far, and the best match will be one cell south.
        center.lat -= resolution;
    } else if (lat - half_res > center.lat && center.lat + resolution < kLatMaxDegrees) {
        // If the proposed code is more than half a cell south of the reference location,
        // it's too far, and the best match will be one cell north.
        center.lat += resolution;
    }
    // How many degrees longitude is the code from the reference?
    if (lon + half_res < center.lon) {
        center.lon -= resolution;
    } else if (lon - half_res > center.lon) {
        center.lon += resolution;
    }
    int gonzo = len + padding_length;
    int returned = Encode(&center, gonzo, code, maxlen);
    // printf("Encode(%s, %f, %f, %d, %d) => [%s]\n", short_code, center.lat, center.lon, len, padding_length, code);
    return returned;
}

int IsValid(const char* code)
{
    char sanitized[256];
    int first_sep = 0;
    int first_pad = 0;
    int len = sanitize(code, sanitized, 256, &first_sep, &first_pad);
    return is_valid(sanitized, len);
}

int IsShort(const char* code)
{
    char sanitized[256];
    int first_sep = 0;
    int first_pad = 0;
    int len = sanitize(code, sanitized, 256, &first_sep, &first_pad);
    return is_short(sanitized, len, first_sep);
}

int IsFull(const char* code)
{
    char sanitized[256];
    int first_sep = 0;
    int first_pad = 0;
    int len = sanitize(code, sanitized, 256, &first_sep, &first_pad);
    return is_full(sanitized, len, first_sep);
}

size_t CodeLength(const char* code)
{
    char sanitized[256];
    int first_sep = 0;
    int first_pad = 0;
    int len = sanitize(code, sanitized, 256, &first_sep, &first_pad);
    return code_length(len, first_sep, first_pad);
}

static int sanitize(const char* code, char* sanitized, int maxlen,
        int* first_sep, int* first_pad)
{
    int len = 0;
    sanitized[0] = '\0';
    *first_sep = 0;
    *first_pad = 0;

    // null code is not valid
    if (!code) {
        return 0;
    }

    int pad_first = -1;
    int pad_last = -1;
    int sep_first = -1;
    int sep_last = -1;
    for (int j = 0; code[j] != '\0'; ++j) {
        int found = 0;
        if (!found && code[j] == kPaddingCharacter) {
            if (pad_first < 0) {
                pad_first = j;
            }
            pad_last = j;
            found = 1;
        }
        if (!found && code[j] == kSeparator) {
            if (sep_first < 0) {
                sep_first = j;
            }
            sep_last = j;
            found = 1;
        }

        // is this a valid character?
        for (int k = 0; !found && kAlphabet[k] != '\0'; ++k) {
            if (toupper(code[j]) == kAlphabet[k]) {
                found = 1;
            }
        }

        // any invalid character messes us up
        if (!found) {
            sanitized[0] = '\0';
            return 0;
        }

        sanitized[len++] = code[j];
    }
    sanitized[len] = '\0';

    // Cannot be empty
    if (len <= 0) {
        return 0;
    }

    // The separator is required.
    if (sep_first < 0) {
        return 0;
    }

    // There can be only one.
    if (sep_last > sep_first) {
        return 0;
    }

    // Is the separator the only character?
    if (len == 1) {
        return 0;
    }

    // Is the separator in an illegal position?
    if (sep_first > kSeparatorPosition || ((sep_first % 2) == 1)) {
        return 0;
    }

    // We can have an even number of padding characters first_sep the separator,
    // but then it must be the final character.
    if (pad_first >= 0) {
        // The first padding character needs to be in an odd position.
        if (pad_first == 0 || pad_first % 2) {
            return 0;
        }

        // Padded codes must not have anything after the separator
        if (len > sep_first + 1) {
            return 0;
        }

        // Get from the first padding character to the separator
        len = pad_first;
        sanitized[len] = '\0';
    }

    // If there are characters after the separator, make sure there isn't just
    // one of them (not legal).
    if ((len - sep_first - 1) == 1) {
        return 0;
    }

    // Make sure the code does not have too many digits in total.
    if ((len - 1) > kMaximumDigitCount) {
        return 0;
    }

    // Make sure the code does not have too many digits after the separator.
    // The number of digits is the length of the code, minus the position of the
    // separator, minus one because the separator position is zero indexed.
    if ((len - sep_first - 1) > (kMaximumDigitCount - kSeparatorPosition)) {
        return 0;
    }

    *first_sep = sep_first;
    *first_pad = pad_first;

    // printf("Sanitize [%s] => [%s] %d %d %d\n", code, sanitized, len, *first_sep, *first_pad);
    return len;
}

static int is_valid(const char* sanitized, int len)
{
    if (len <= 0) {
        return 0;
    }

    return 1;
}

static int is_short(const char* sanitized, int len, int first_sep)
{
    if (len <= 0) {
        return 0;
    }

    // If there are less characters than expected first_sep the SEPARATOR.
    if (first_sep >= kSeparatorPosition) {
        return 0;
    }

    return 1;
}

static int is_full(const char* sanitized, int len, int first_sep)
{
    if (len <= 0) {
        return 0;
    }

    // If there are less characters than expected first_sep the SEPARATOR.
    if (first_sep < kSeparatorPosition) {
        return 0;
    }

    if (len > 0) {
        // Work out what the first latitude character indicates for latitude.
        size_t firstLatValue = get_alphabet_position(toupper(sanitized[0]));
        firstLatValue *= kEncodingBase;
        if (firstLatValue >= kLatMaxDegrees * 2) {
            // The code would decode to a latitude of >= 90 degrees.
            return 0;
        }
    }
    if (len > 1) {
        // Work out what the first longitude character indicates for longitude.
        size_t firstLonValue = get_alphabet_position(toupper(sanitized[1]));
        firstLonValue *= kEncodingBase;
        if (firstLonValue >= kLonMaxDegrees * 2) {
            // The code would decode to a longitude of >= 180 degrees.
            return 0;
        }
    }

    return 1;
}

static int decode(char* sanitized, int len, int first_sep, int first_pad, OLC_CodeArea* decoded)
{
    // HACK: remember whether we shifted away the separator, so that we can
    // correct the code length at the end.
    int fixed = 0;

    // Make a copy that doesn't have the separator and stops at the first padding
    // character.
    if (first_sep >= 0) {
        for (int j = first_sep + 1; sanitized[j] != '\0'; ++j) {
            sanitized[j-1] = sanitized[j];
        }
        sanitized[--len] = '\0';
        if (first_pad >= first_sep) {
            --first_pad;
        }
        fixed = 1;
    }
    int top = len;
    if (first_pad >= 0) {
        top = first_pad;
    }
    if (top > kPairCodeLength) {
        top = kPairCodeLength;
    }
    // printf("Decode [%s] [%*.*s] %d %d\n", sanitized, top, top, sanitized, first_sep, first_pad);
    double resolution_degrees = kEncodingBase;
    double lat = 0.0;
    double lon = 0.0;
    double lat_high = 0.0;
    double lon_high = 0.0;
    // Up to the first 10 characters are encoded in pairs. Subsequent characters
    // represent grid squares.
    for (size_t i = 0; i < top; resolution_degrees /= kEncodingBase) {
        double value;

        // The character at i represents latitude. Retrieve it and convert to
        // degrees (positive range).
        value = get_alphabet_position(toupper(sanitized[i]));
        value *= resolution_degrees;
        lat += value;
        lat_high = lat + resolution_degrees;
        if (i == top) {
            break;
        }
        ++i;

        // The character at i + 1 represents longitude. Retrieve it and convert to
        // degrees (positive range).
        value = get_alphabet_position(toupper(sanitized[i]));
        value *= resolution_degrees;
        lon += value;
        lon_high = lon + resolution_degrees;
        if (i == top) {
            break;
        }
        ++i;
    }
    if (first_pad > kPairCodeLength) {
        // Now do any grid square characters.
        // Adjust the resolution back a step because we need the resolution of the
        // entire grid, not a single grid square.
        resolution_degrees *= kEncodingBase;

        // With a grid, the latitude and longitude resolutions are no longer equal.
        double lat_resolution = resolution_degrees;
        double lon_resolution = resolution_degrees;

        // Decode only up to the maximum digit count.
        top = len;
        // printf("MIN: %lu %lu\n", kMaximumDigitCount, top);
        if (top > kMaximumDigitCount) {
            top = kMaximumDigitCount;
        }
        // printf("ENTER: %lu %lu\n", kPairCodeLength, top);
        for (size_t i = kPairCodeLength; i < top; ++i) {
            // Get the value of the character at i and convert it to the degree value.
            size_t value = get_alphabet_position(toupper(sanitized[i]));
            size_t row = value / kGridCols;
            size_t col = value % kGridCols;
            // printf("GONZO: %lu %lu %lu %lu\n", i, value, row, col);
            // Lat and lng grid sizes shouldn't underflow due to maximum code
            // length enforcement, but a hypothetical underflow won't cause
            // fatal errors here.
            lat_resolution /= kGridRows;
            lon_resolution /= kGridCols;
            lat += row * lat_resolution;
            lon += col * lon_resolution;
            lat_high = lat + lat_resolution;
            lon_high = lon + lon_resolution;
        }
    }
    // printf("CALL LEN [%s]: %d %d %d\n", sanitized, len, first_sep, first_pad);
    len = code_length(len, first_sep, first_pad);
    // printf("GOT LEN %d\n", len);
    decoded->lat_lo = lat - kLatMaxDegrees;
    decoded->lon_lo = lon - kLonMaxDegrees;
    decoded->lat_hi = lat_high - kLatMaxDegrees;
    decoded->lon_hi = lon_high - kLonMaxDegrees;
    decoded->code_len = len + fixed;
    return len;
}

static size_t code_length(int len, int first_sep, int first_pad)
{
    if (first_sep >= 0) {
        --len;
        --first_pad;
    }
    if (first_pad >= 0) {
        len -= first_pad;
    }
    return len;
}
