#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <sstream>
#include <memory>
#include <functional>

#include "../command.hxx"
#include "../argument.hxx"
#include "../interp.hxx"
#include "../common.hxx"

using namespace std;

namespace tglng {
  template<typename Operation, bool Div>
  class ArithmeticCommand: public Command {
    auto_ptr<Command> lhs, rhs;
    Operation op;

  public:
    ArithmeticCommand(Command* left,
                      auto_ptr<Command>& l,
                      auto_ptr<Command>& r)
    : Command(left), lhs(l), rhs(r)
    { }

    virtual bool exec(wstring& dst, Interpreter& interp) {
      wstring lstr, rstr;
      signed lint, rint;
      if (!interp.exec(lstr, lhs.get())) return false;
      if (!parseInteger(lint, lstr)) {
        wcerr << L"Invalid integer for operator LHS: " << lstr << endl;
        return false;
      }
      if (!interp.exec(rstr, rhs.get())) return false;
      if (!parseInteger(rint, rstr)) {
        wcerr << L"Invalid integer for operator RHS: " << rstr << endl;
        return false;
      }

      if (Div && !rint) {
        wcerr << L"Divide by zero." << endl;
        return false;
      }

      wostringstream tmp;
      tmp << (signed)(op(lint,rint));
      dst = tmp.str();
      return true;
    }
  };

  template<typename Operation, bool Div>
  class ArithmeticParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const wstring& text,
                              unsigned& offset) {
      auto_ptr<Command> lhs, rhs;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.a(lhs), a.a(rhs)]) return ParseError;

      out = new ArithmeticCommand<Operation,Div>(out, lhs, rhs);
      return ContinueParsing;
    }
  };

  static GlobalBinding<ArithmeticParser<plus<signed>,
                                        false> >
  _addParser(L"num-add");
  static GlobalBinding<ArithmeticParser<minus<signed>,
                                        false> >
  _subParser(L"num-sub");
  static GlobalBinding<ArithmeticParser<multiplies<signed>,
                                        false> >
  _mulParser(L"num-mul");
  static GlobalBinding<ArithmeticParser<divides<signed>,
                                        true> >
  _divParser(L"num-div");
  static GlobalBinding<ArithmeticParser<modulus<signed>,
                                        true> >
  _modParser(L"num-mod");
  static GlobalBinding<ArithmeticParser<equal_to<signed>,
                                        false> >
  _equParser(L"num-equ");
  static GlobalBinding<ArithmeticParser<not_equal_to<signed>,
                                        false> >
  _neqParser(L"num-neq");
  static GlobalBinding<ArithmeticParser<less<signed>,
                                        false> >
  _sltParser(L"num-slt");
  static GlobalBinding<ArithmeticParser<greater<signed>,
                                        false> >
  _sgtParser(L"num-sgt");
  static GlobalBinding<ArithmeticParser<less_equal<signed>,
                                        false> >
  _leqParser(L"num-leq");
  static GlobalBinding<ArithmeticParser<greater_equal<signed>,
                                        false> >
  _geqParser(L"num-geq");
}
