#include <algorithm>
#include <array>

using RGB_t = std::array<int,3>;
using RGBW_t = std::array<int,4>;

template<typename From, typename To>
To convert(From & rgbColor);

template<>
RGB_t convert<RGB_t,RGB_t>(RGB_t & rgbColor) { return rgbColor; }

template<>
RGBW_t convert<RGB_t,RGBW_t>(RGB_t & rgbColor) { 
  const auto min_max_pair = std::minmax_element(rgbColor.begin(), rgbColor.end());
  //const auto luma = std::accumulate(rgbColor.begin(), rgbColor.end()) / rgbColor.size();
  return {rgbColor[0] - *min_max_pair.first, rgbColor[1] - *min_max_pair.first, rgbColor[2] - *min_max_pair.first, *min_max_pair.first};
}

template<>
RGB_t convert<RGBW_t,RGB_t>(RGBW_t & rgbwColor) { 
  return {rgbwColor[0] + rgbwColor[3], rgbwColor[1] + rgbwColor[3], rgbwColor[2] + rgbwColor[3]};
}
