#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <memory>

#include "../command.hxx"
#include "../argument.hxx"
#include "../interp.hxx"
#include "basic_parsers.hxx"
#include "../common.hxx"

using namespace std;

namespace tglng {
  template<typename Logic>
  class LogicalCommand: public Command {
    Logic logic;
    auto_ptr<Command> lhs, rhs;

  public:
    LogicalCommand(Command* left,
                   auto_ptr<Command>& l,
                   auto_ptr<Command>& r)
    : Command(left), lhs(l), rhs(r)
    { }

    virtual bool exec(wstring& dst, Interpreter& interp) {
      wstring lstr, rstr;
      bool lb, rb;
      if (!interp.exec(lstr, lhs.get())) return false;
      lb = parseBool(lstr);
      if (logic.needRhs(lb)) {
        if (!interp.exec(rstr, rhs.get())) return false;
        rb = parseBool(rstr);
      }

      dst = (logic.eval(lb, rb)? L"1" : L"0");
      return true;
    }
  };

  template<typename Logic>
  class LogicalParser: public BinaryCommandParser<LogicalCommand<Logic> > {
  };

  struct LogicalAnd {
    bool needRhs(bool b) { return b; }
    bool eval(bool l, bool r) { return l && r; }
  };
  struct LogicalOr {
    bool needRhs(bool b) { return !b; }
    bool eval(bool l, bool r) { return l || r; }
  };
  struct LogicalXor {
    bool needRhs(bool b) { return true; }
    bool eval(bool l, bool r) { return l ^ r; }
  };

  static GlobalBinding<LogicalParser<LogicalAnd> > _andParser(L"logical-and");
  static GlobalBinding<LogicalParser<LogicalOr > >  _orParser(L"logical-or");
  static GlobalBinding<LogicalParser<LogicalXor> > _xorParser(L"logical-xor");

  class LogicalNot: public Command {
    auto_ptr<Command> sub;

  public:
    LogicalNot(Command* left, auto_ptr<Command>& s)
    : Command(left), sub(s) {}

    virtual bool exec(wstring& out, Interpreter& interp) {
      wstring tmp;
      if (!interp.exec(tmp, sub.get())) return false;

      out = (!parseBool(tmp)? L"1" : L"0");
      return true;
    }
  };

  static GlobalBinding<UnaryCommandParser<LogicalNot> >
  _notParser(L"logical-not");
}
