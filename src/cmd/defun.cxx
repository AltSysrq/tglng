#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <cstdlib>
#include <memory>
#include <map>

#include "../command.hxx"
#include "../function.hxx"
#include "../interp.hxx"
#include "../argument.hxx"
#include "../common.hxx"

using namespace std;

namespace tglng {
  struct UserFunction {
    auto_ptr<Command> body;
    wstring outputs, inputs;
  };

  static bool executeUserFunction(wstring* out, const wstring* in,
                                  Interpreter& interp, unsigned ref) {
    UserFunction* uf = (UserFunction*)interp.external(ref);
    //Backup all registers
    Interpreter::registers_t regbak(interp.registers);

    //Bind inputs
    for (unsigned i = 0; i < uf->inputs.size(); ++i)
      interp.registers[uf->inputs[i]] = in[i];

    //Call main command
    bool result = interp.exec(out[0], uf->body.get());

    //If successful, bind outputs
    for (unsigned i = 0; result && i < uf->outputs.size(); ++i)
      out[i] = interp.registers[uf->outputs[i]];

    //Restore registers
    interp.registers = regbak;

    return result;
  }

  class BasicFunctionDefiner {
  protected:
    bool defineFunction(Interpreter& interp,
                        wchar_t shortName /* NUL=none */,
                        const wstring& longName,
                        const wstring& outputs,
                        const wstring& inputs,
                        Command* body,
                        const wstring& text,
                        unsigned nameOffset) {
      if (interp.commandsL.count(longName)) {
        interp.error(wstring(L"Command name already in use: ") + longName,
                     text, nameOffset);
        return false;
      }

      UserFunction* uf = new UserFunction;
      uf->body.reset(body);
      uf->outputs = outputs;
      uf->inputs = inputs;
      unsigned ref = interp.bindExternal(uf);

      interp.commandsL[longName] =
        new FunctionParser(
          Function(outputs.size()+1, inputs.size(), executeUserFunction, ref));
      if (shortName)
        interp.commandsS[shortName] = interp.commandsL[longName];

      return true;
    }
  };

  class DefunParser: public CommandParser, private BasicFunctionDefiner {
  public:
    ParseResult parse(Interpreter& interp,
                      Command*& out,
                      const wstring& text,
                      unsigned& offset) {
      wstring name, outputs, inputs;
      unsigned nameOffset;
      wchar_t shortName = 0;
      auto_ptr<Command> body;

      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.to(name, L'#') >> nameOffset,
             -(a.x(L':'), a.h(shortName)),
             -(a.x(L'['), a.to(outputs, L']') | a.x(L']')),
             -(a.x(L'('), a.to(inputs, L')') | a.x(L')')),
             a.a(body)])
        return ParseError;

      if  (defineFunction(interp,
                          shortName,
                          name,
                          outputs,
                          inputs,
                          body.get(),
                          text,
                          nameOffset)) {
        body.release();
        return ContinueParsing;
      } else {
        return ParseError;
      }
    }
  };

  static GlobalBinding<DefunParser> _defun(L"defun");
}
