#ifndef CMD_LONG_MODE_HXX_
#define CMD_LONG_MODE_HXX_

#include <string>

#include "../command.hxx"

namespace tglng {
  /**
   * Reads contiguous alphanumeric, hyphen, and underscore characters into a
   * command name, and then parses that command.
   */
  class LongModeCmdParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter&, Command*&,
                              const std::wstring&, unsigned&);

    /**
     * Returns whether the given character is considered a name character, for
     * the purposes of long mode.
     */
    static bool isname(wchar_t);
  };
}

#endif /* CMD_LONG_MODE_HXX_ */
