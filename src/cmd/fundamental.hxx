#ifndef CMD_FUNDAMENTAL_HXX_
#define CMD_FUNDAMENTAL_HXX_

#include <string>

#include "../command.hxx"

namespace tglng {
  /**
   * Command which evaluates to a fixed string.
   */
  class SelfInsertCommand: public Command {
    const std::wstring value;

  public:
    /**
     * Creates a SelfInsertCommand which evaluates to a string of length one
     * consisting solely of the given character.
     */
    SelfInsertCommand(Command*, wchar_t);
    /**
     * Creates a SelfInsertCommand which evaluates to the given string.
     */
    SelfInsertCommand(Command*, const std::wstring&);

    virtual bool exec(std::wstring&, Interpreter&);
  };

  /**
   * Parser for SelfInsertCommand.
   */
  class SelfInsertParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter&, Command*&,
                              const std::wstring&, unsigned& offset);
  };

  /**
   * Do-nothing parser.
   *
   * On parse, it simply skips the command character, causing the command to be
   * treated as a no-op.
   */
  class NullParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter&, Command*&,
                              const std::wstring&, unsigned& offset);
  };
}

#endif /* CMD_FUNDAMENTAL_HXX_ */
