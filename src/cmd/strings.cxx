#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <sstream>
#include <memory>

#include "../command.hxx"
#include "../interp.hxx"
#include "../argument.hxx"
#include "../common.hxx"

using namespace std;

namespace tglng {
  template<typename Operator>
  class StringComparison: public Command {
    Operator op;
    auto_ptr<Command> lhs, rhs;

  public:
    StringComparison(Command* left,
                     auto_ptr<Command>& l,
                     auto_ptr<Command>& r)
    : Command(left), lhs(l), rhs(r) {}

    virtual bool exec(wstring& out, Interpreter& interp) {
      wstring lstr, rstr;
      if (!interp.exec(lstr, lhs.get()) ||
          !interp.exec(rstr, rhs.get())) return false;

      out = op(lstr,rstr)? L"1" : L"0";
      return true;
    }
  };

  template<typename Operator>
  class StringComparisonParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const wstring& text,
                              unsigned& offset) {
      auto_ptr<Command> lhs, rhs;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.a(lhs), a.a(rhs)]) return ParseError;

      out = new StringComparison<Operator>(out, lhs, rhs);
      return ContinueParsing;
    }
  };

  static GlobalBinding<StringComparisonParser<equal_to<wstring> > >
  _strequParser(L"str-equ");
  static GlobalBinding<StringComparisonParser<less<wstring> > >
  _strsltParser(L"str-slt");
  static GlobalBinding<StringComparisonParser<greater<wstring> > >
  _strsgtParser(L"str-sgt");
}
