#ifndef REGEX_HXX_
#define REGEX_HXX_

#include <string>

namespace tglng {
///Indicates that regular expressions are not supported
#define TGLNG_REGEX_NONE   0
///Indicates that POSIX regular expressions are being used
#define TGLNG_REGEX_POSIX  1
///Indicates that 8-bit PCRE regular expressions are being used
#define TGLNG_REGEX_PCRE8  2
///Indicates that 16-bit PCRE regular expressions are being used
#define TGLNG_REGEX_PCRE16 3
  /**
   * Defined to one of the TGLNG_REGEX_* constants above.
   *
   * Indicates what level of support for regular expressions is present.
   */
  extern const unsigned regexLevel;
  /**
   * Indicates the human-readable name of regexLevel.
   */
  extern const std::wstring regexLevelName;

  ///Internally used by Regex
  struct RegexData;

  /**
   * Encapsulates the various possible regular expression support levels into
   * one, consistent RAII interface.
   */
  class Regex {
    RegexData& data;

    Regex();
    Regex(const Regex&);

  public:
    /**
     * Constructs a Regex on the given pattern and options.
     *
     * If the pattern is invalid, no exception is thrown; rather, operator
     * bool() will return false.
     */
    Regex(const std::wstring& pattern, const std::wstring& options);

    ~Regex();

    /**
     * Returns whether the Regex is currently in a valid state.
     *
     * This becomes false only in error conditions.
     */
    operator bool() const;
    inline bool operator!() const { return !(bool)*this; }

    /**
     * If operator bool() returns false, prints a human-readable error message.
     */
    void showWhy() const;

    /**
     * Returns the character offset within the input pattern where a parse
     * error occurred, or zero if unknown.
     */
    unsigned where() const;

    /**
     * Sets a new input string for this regex.
     */
    void input(const std::wstring&);

    /**
     * Tries to match the current input to the pattern. Successive calls will
     * match against latter parts of the text.
     *
     * @return Whether the match succeeded
     */
    bool match();

    /**
     * Returns the number of groups matched by the last call to match(),
     * including the "whole pattern" group.
     */
    unsigned groupCount() const;

    /**
     * Retrieves the text of the group at the given index, where 0 indicates the
     * whole string portion which was matched.
     */
    void group(std::wstring&, unsigned ix) const;

    /**
     * Retrieves the portion of the input string which was skipped by the last
     * call to match().
     */
    void head(std::wstring&) const;
    /**
     * Retrieves the portion of the input string which has not been matched.
     *
     * This value is undefined after match() has returned false.
     */
    void tail(std::wstring&) const;
  };
}

#endif /* REGEX_HXX_ */
