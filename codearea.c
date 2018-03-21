#include "codearea.h"

static const double kLatMaxDegrees =  90.0;
static const double kLonMaxDegrees = 180.0;

void OLC_CodeArea_GetCenter(const OLC_CodeArea* codearea, OLC_LatLon* center)
{
    center->lat = codearea->lat_lo + (codearea->lat_hi - codearea->lat_lo) / 2.0;
    if (center->lat > kLatMaxDegrees) {
        center->lat = kLatMaxDegrees;
    }

    center->lon = codearea->lon_lo + (codearea->lon_hi - codearea->lon_lo) / 2.0;
    if (center->lon > kLonMaxDegrees) {
        center->lon = kLonMaxDegrees;
    }
}
