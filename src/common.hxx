#ifndef COMMON_HXX_
#define COMMON_HXX_

#include <string>
#include <vector>

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

  /**
   * Parses the given string as a boolean, and returns whether it is considered
   * true.
   *
   * This function always succeeds.
   *
   * @param str The string to convert.
   * @return Whether the string is considered true.
   */
  bool parseBool(const std::wstring& str);

  /**
   * Convenience function to convert an integer to a string.
   *
   * @param value The integer to convert.
   * @return The string representation of the integer.
   */
  std::wstring intToStr(signed);

  /**
   * Converts a wstring into a narrow, NTBS stored within the given vector.
   *
   * If end is non-NULL, a pointer to the "real" terminating NULL is written
   * there.
   *
   * Returns whether conversion succeeded.
   */
  bool wstrtontbs(std::vector<char>&, const std::wstring&,
                  const char** end = NULL);

  /**
   * Converts a wstring into a narrow string.
   *
   * Returns whether conversion succeeded.
   */
  bool wstrtostr(std::string&, const std::wstring&);

  /**
   * Converts a narrow NTBS into a wstring.
   *
   * If end is non-NULL, the string is considered to end there instead of at
   * the first NUL.
   *
   * Returns whether conversion succeeded.
   */
  bool ntbstowstr(std::wstring&, const char*, const char* end = NULL);

  /**
   * Converts a string into a wstring.
   *
   * Returns whether conversion succeeded.
   */
  bool strtowstr(std::wstring&, const std::string&);
}

#endif /* COMMON_HXX_ */
