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
}

#endif /* CMD_FUNDAMENTAL_HXX_ */
