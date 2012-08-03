#ifndef COMMON_HXX_
#define COMMON_HXX_

#include <string>

//Exit codes (other than EXIT_SUCCESS)
#define EXIT_PARSE_ERROR_IN_USER_LIBRARY 1
#define EXIT_PARSE_ERROR_IN_INPUT 2
#define EXIT_EXEC_ERROR_IN_USER_LIBRARY 3
#define EXIT_EXEC_ERROR_IN_INPUT 4
#define EXIT_PLATFORM_ERROR 5
#define EXIT_INCORRECT_USAGE 254
#define EXIT_THE_SKY_IS_FALLING 255

namespace tglng {
  /**
   * Parses the given string as a signed integer, using TglNG's rules.
   *
   * @param dst The value parsed. If unsuccessful, its content is undefined.
   * @param str The string to parse.
   * @param offset The first index of the string to use, or 0 by default.
   * @param end If non-NULL, allow parsing to stop prematurely and write the
   * index of the first unused character into this value. NULL by default.
   * @return Whether parsing succeeded.
   */
  bool parseInteger(signed& dst, const std::wstring& str,
                    unsigned off = 0, unsigned* end = NULL);
}

#endif /* COMMON_HXX_ */