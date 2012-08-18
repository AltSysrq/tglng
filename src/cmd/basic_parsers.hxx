#ifndef BASIC_PARSERS_HXX_
#define BASIC_PARSERS_HXX_

#include "../command.hxx"
#include "../argument.hxx"
#include "../function.hxx"
#include "fundamental.hxx"

namespace tglng {
  /**
   * CommandParser for commands which take just a single Arithmetic argument.
   */
  template<typename C>
  class UnaryCommandParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp,
                              Command*& out,
                              const std::wstring& text,
                              unsigned& offset) {
      std::auto_ptr<Command> sub;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.a(sub)]) return ParseError;

      out = new C(out, sub);
      return ContinueParsing;
    }

    virtual bool function(Function& fun) const {
      fun = Function(1, 1, &adapter);
      return true;
    }

  private:
    static bool adapter(std::wstring* out, const std::wstring* in,
                        Interpreter& interp, unsigned) {
      std::auto_ptr<Command> sub(
        new SelfInsertCommand(NULL, in[0]));
      C c(NULL, sub);
      return c.exec(out[0], interp);
    }
  };

  /**
   * Provides the common fields for unary arithmetic commands.
   */
  class UnaryCommand: public Command {
  protected:
    std::auto_ptr<Command> sub;

    UnaryCommand(Command* left, std::auto_ptr<Command>& sub_)
    : Command(left), sub(sub_) {}
  };

  /**
   * CommandParser for commands which take two Arithmetic arguments.
   */
  template<typename C>
  class BinaryCommandParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp,
                              Command*& out,
                              const std::wstring& text,
                              unsigned& offset) {
      std::auto_ptr<Command> lhs, rhs;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.a(lhs), a.a(rhs)]) return ParseError;

      out = new C(out, lhs, rhs);
      return ContinueParsing;
    }

    virtual bool function(Function& fun) const {
      fun = Function(1, 2, &adapter);
      return true;
    }

  private:
    static bool adapter(std::wstring& out, const std::wstring* in,
                        Interpreter& interp, unsigned) {
      std::auto_ptr<Command>
        lhs(new SelfInsertCommand(NULL, in[0])),
        rhs(new SelfInsertCommand(NULL, in[1]));
      C c(NULL, lhs, rhs);
      return c.exec(out[0], interp);
    }
  };

  /**
   * Provides the common fields for binary arithmetic commands.
   */
  class BinaryCommand: public Command {
  protected:
    std::auto_ptr<Command> lhs, rhs;

    BinaryCommand(Command* left,
                  std::auto_ptr<Command>& lhs_,
                  std::auto_ptr<Command>& rhs_)
    : Command(left), lhs(lhs_), rhs(rhs_) {}
  };
}

#endif /* BASIC_PARSERS_HXX_ */
