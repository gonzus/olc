#ifndef OLC_OPENLOCATIONCODE_H_
#define OLC_OPENLOCATIONCODE_H_

#include "codearea.h"

int Encode(const OLC_LatLon* location, size_t code_length,
           char* code, int maxlen);
int EncodeDefault(const OLC_LatLon* location, char* code, int maxlen);
int Decode(const char* code, OLC_CodeArea* decoded);

int Shorten(const char* code, const OLC_LatLon* reference_location,
            char* buf, int maxlen);
int RecoverNearest(const char* short_code, const OLC_LatLon* reference_location,
                   char* buf, int maxlen);
size_t CodeLength(const char* code);

int IsValid(const char* code);
int IsShort(const char* code);
int IsFull(const char* code);

#endif
