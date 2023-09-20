#include <spdlog/spdlog.h>

// needed for spdlog colors on windows -- no point in including all of wincon.h
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#ifdef _WIN32
  #define FOREGROUND_BLUE 0x0001
  #define FOREGROUND_GREEN 0x0002
  #define FOREGROUND_RED 0x0004
  #define FOREGROUND_WHITE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
  #define FOREGROUND_INTENSITY 0x0008
  #define BACKGROUND_BLUE 0x0010
  #define BACKGROUND_GREEN 0x0020
  #define BACKGROUND_RED 0x0040
  #define BACKGROUND_INTENSITY 0x0080
  #define COMMON_LVB_LEADING_BYTE 0x0100
  #define COMMON_LVB_TRAILING_BYTE 0x0200
  #define COMMON_LVB_GRID_HORIZONTAL 0x0400
  #define COMMON_LVB_GRID_LVERTICAL 0x0800
  #define COMMON_LVB_GRID_RVERTICAL 0x1000
  #define COMMON_LVB_REVERSE_VIDEO 0x4000
  #define COMMON_LVB_UNDERSCORE 0x8000
#endif  // _WIN32
// NOLINTEND(cppcoreguidelines-macro-usage)

namespace hvk {

void configure_logger();

}  // namespace hvk
