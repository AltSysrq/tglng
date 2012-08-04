#ifndef CMD_REGISTERS_HXX_
#define CMD_REGISTERS_HXX_

#include <string>

#include "../command.hxx"

namespace tglng {
  /**
   * Command which results in the value of a certain register.
   */
  class ReadRegister: public Command {
    wchar_t reg;

  public:
    ReadRegister(Command*, wchar_t);
    virtual bool exec(std::wstring&, Interpreter&);
  };
}

#endif /* CMD_REGISTERS_HXX_ */
