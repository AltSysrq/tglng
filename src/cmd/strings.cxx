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

  class StringSearch: public Command {
    auto_ptr<Command> needle, haystack;

  public:
    StringSearch(Command* left,
                 auto_ptr<Command>& n,
                 auto_ptr<Command>& h)
    : Command(left), needle(n), haystack(h)
    { }

    virtual bool exec(wstring& out, Interpreter& interp) {
      wstring ns, hs;
      if (!needle  ->exec(ns, interp)) return false;
      if (!haystack->exec(hs, interp)) return false;

      size_t result = hs.find(ns);
      if (result == wstring::npos)
        //Not found
        out = L"";
      else
        out = intToStr((signed)result);

      return true;
    }
  };

  class StringSearchParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const wstring& text,
                              unsigned& offset) {
      auto_ptr<Command> needle, haystack;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.a(needle), a.a(haystack)])
        return ParseError;

      out = new StringSearch(out, needle, haystack);
      return ContinueParsing;
    }
  };

  static GlobalBinding<StringSearchParser> _stringSearchParser(L"str-str");

  class StringIndex: public Command {
    auto_ptr<Command> begin, end;
    bool treatEndAsLength;
    AutoSection string;

  public:
    StringIndex(Command* left,
                auto_ptr<Command>& begin_,
                auto_ptr<Command>& end_,
                bool treatEndAsLength_,
                Section string_)
    : Command(left),
      begin(begin_), end(end_),
      treatEndAsLength(treatEndAsLength_),
      string(string_)
    { }

    virtual bool exec(wstring& out, Interpreter& interp) {
      wstring sb, se, str, strr;
      signed ib, ie;

      //Get the parms
      if (!begin->exec(sb, interp) ||
          (end.get() && !end->exec(se, interp)) ||
          (string.left && !string.left->exec(str, interp)) ||
          (string.right && !string.right->exec(strr, interp)))
        return false;

      //Concat section parts
      str += strr;

      //Convert integers
      if (!parseInteger(ib, sb)) {
        wcerr << L"Invalid integer: " << sb << endl;
        return false;
      }

      if (se.empty())
        //Implicitly one character
        ie = ib+1;
      else {
        if (!parseInteger(ie, se)) {
          wcerr << L"Invalid integer: " << se << endl;
          return false;
        }

        //Add ib if in length mode
        if (treatEndAsLength)
          ie += ib;
      }

      //Negative indices are relative to the end plus one
      if (ib < 0) ib += str.size()+1;
      if (ie < 0) ie += str.size()+1;

      //Clamp indices
      if (ib < 0) ib = 0;
      if (ib > (signed)str.size()) ib = str.size()-1;
      if (ie < ib) ie = ib;
      if (ie > (signed)str.size()) ie = str.size()-1;

      //OK
      out = str.substr(ib, ie-ib);
      return true;
    }
  };

  class StringIndexParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const wstring& text, unsigned& offset) {
      auto_ptr<Command> begin, end;
      AutoSection string;
      bool length = false;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.a(begin),
             (-a.x(length, L'.'), a.a(end)) -=
             a.s(string)])
        return ParseError;

      out = new StringIndex(out, begin, end, length, string);
      string.clear(); //Now owned by out
      return ContinueParsing;
    }
  };

  static GlobalBinding<StringIndexParser> _stringIndexParser(L"str-ix");
}
