// TL47

#include "atlas/grids/reduced_gg/reduced_gg.h"

namespace atlas {
namespace grids {
namespace reduced_gg {

void N24::construct()
{
  int N=24;
  int lon[] = {
    20,
    25,
    36,
    40,
    45,
    48,
    54,
    60,
    64,
    72,
    80,
    80,
    90,
    90,
    96,
    96,
    96,
    96,
    96,
    96,
    96,
    96,
    96,
    96
  };
  double lat[] = {
    87.1590945558628505,
    83.4789366693171644,
    79.7770456548256419,
    76.0702444625451335,
    72.3615810293448476,
    68.6520167895174893,
    64.9419494887575155,
    61.2315731880771352,
    57.5209937979699646,
    53.8102740319414252,
    50.0994534129868470,
    46.3885581116054269,
    42.6776061726049036,
    38.9666104694540252,
    35.2555804613681829,
    31.5445232840216754,
    27.8334444519932376,
    24.1223483260879874,
    20.4112384335677852,
    16.7001176938426745,
    12.9889885820881474,
     9.2778532515078656,
     5.5667136279135834,
     1.8555714859932551
  };
  setup_lat_hemisphere(N,lat,lon,DEG);
}

} // namespace reduced_gg
} // namespace grids
} // namespace atlas
