#include "codearea.h"

static const double kLatMaxDegrees =  90.0;
static const double kLonMaxDegrees = 180.0;

void OLC_CodeArea_GetCenter(const OLC_CodeArea* area, OLC_LatLon* center)
{
    center->lat = area->lo.lat + (area->hi.lat - area->lo.lat) / 2.0;
    if (center->lat > kLatMaxDegrees) {
        center->lat = kLatMaxDegrees;
    }

    center->lon = area->lo.lon + (area->hi.lon - area->lo.lon) / 2.0;
    if (center->lon > kLonMaxDegrees) {
        center->lon = kLonMaxDegrees;
    }
}
