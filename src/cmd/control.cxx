#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <memory>

#include "../command.hxx"
#include "../argument.hxx"
#include "../interp.hxx"
#include "../common.hxx"

using namespace std;

namespace tglng {
  class If: public Command {
    AutoSection condition, then, otherwise;

  public:
    If(Command* left,
       const Section& c,
       const Section& t,
       const Section& o)
    : Command(left),
      condition(c), then(t), otherwise(o)
    { }

    virtual bool exec(wstring& dst, Interpreter& interp) {
      wstring cond;
      if (!condition.exec(cond, interp)) return false;
      return (parseBool(cond)? then : otherwise).exec(dst, interp);
    }
  };

  class IfParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const wstring& text, unsigned& offset) {
      AutoSection condition, then, otherwise;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.s(condition), a.s(then), -a.s(otherwise)])
        return ParseError;

      out = new If(out, condition, then, otherwise);
      condition.clear();
      then.clear();
      otherwise.clear();
      return ContinueParsing;
    }
  };

  static GlobalBinding<IfParser> _ifParser(L"if");

  class FalseCoalesce: public Command {
    AutoSection lhs, rhs;

  public:
    FalseCoalesce(Command* left,
                  const Section& l,
                  const Section& r)
    : Command(left), lhs(l), rhs(r) {}

    virtual bool exec(wstring& dst, Interpreter& interp) {
      if (!lhs.exec(dst, interp)) return false;
      return parseBool(dst) || rhs.exec(dst, interp);
    }
  };

  class FalseCoalesceParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const wstring& text, unsigned& offset) {
      AutoSection lhs, rhs;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.s(lhs), a.s(rhs)]) return ParseError;

      out = new FalseCoalesce(out, lhs, rhs);
      lhs.clear();
      rhs.clear();
      return ContinueParsing;
    }
  };

  static GlobalBinding<FalseCoalesceParser>
  _falseCoalesceParser(L"false-coalesce");
}
