#ifndef OLC_CODEAREA_H_
#define OLC_CODEAREA_H_

#include <stdlib.h>

typedef struct OLC_LatLon {
    double lat;
    double lon;
} OLC_LatLon;

typedef struct OLC_CodeArea {
    OLC_LatLon lo;
    OLC_LatLon hi;
    size_t len;
} OLC_CodeArea;

void OLC_CodeArea_GetCenter(const OLC_CodeArea* codearea, OLC_LatLon* center);

#endif
