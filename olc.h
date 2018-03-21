#ifndef OLC_OPENLOCATIONCODE_H_
#define OLC_OPENLOCATIONCODE_H_

// A pair of doubles representing latitude / longitude
typedef struct OLC_LatLon {
    double lat;
    double lon;
} OLC_LatLon;

// An area defined by two corners (lo and hi) and a code length
typedef struct OLC_CodeArea {
    OLC_LatLon lo;
    OLC_LatLon hi;
    size_t len;
} OLC_CodeArea;

// Gets the center coordinates for an area
void GetCenter(const OLC_CodeArea* area, OLC_LatLon* center);

// Get the effective length for a code
size_t CodeLength(const char* code);

// Checkers for the three obviously-named conditions
int IsValid(const char* code);
int IsShort(const char* code);
int IsFull(const char* code);

// Encode a location with a given code length (which indicates precision) into
// an OLC
int Encode(const OLC_LatLon* location, size_t code_length,
           char* code, int maxlen);

// Encode a location with a default code length into an OLC
int EncodeDefault(const OLC_LatLon* location, char* code, int maxlen);

// Decode an OLC into the original location
int Decode(const char* code, OLC_CodeArea* decoded);

// Compute a (shorter) OLC for a given code and a reference location
int Shorten(const char* code, const OLC_LatLon* reference,
            char* buf, int maxlen);

// Given a shorter OLC and a reference location, compute the original (full
// length) OLC
int RecoverNearest(const char* short_code, const OLC_LatLon* reference,
                   char* buf, int maxlen);

#endif
