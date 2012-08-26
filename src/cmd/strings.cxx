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
#include "basic_parsers.hxx"

using namespace std;

namespace tglng {
  template<typename Operator>
  class StringComparison: public BinaryCommand {
    Operator op;

  public:
    StringComparison(Command* left,
                     auto_ptr<Command>& l,
                     auto_ptr<Command>& r)
    : BinaryCommand(left, l, r) {}

    virtual bool exec(wstring& out, Interpreter& interp) {
      wstring lstr, rstr;
      if (!interp.exec(lstr, lhs.get()) ||
          !interp.exec(rstr, rhs.get())) return false;

      out = op(lstr,rstr)? L"1" : L"0";
      return true;
    }
  };

  template<typename Operator>
  class StringComparisonParser:
  public BinaryCommandParser<StringComparison<Operator> > {
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
      if (!interp.exec(ns, needle  .get())) return false;
      if (!interp.exec(hs, haystack.get())) return false;

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
      if ((string.left && !interp.exec(str, string.left)) ||
          !interp.exec(sb, begin.get()) ||
          (end.get() && !interp.exec(se, end.get())) ||
          (string.right && !interp.exec(strr, string.right)))
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

      //Negative indices are relative to the end (plus one for end)
      if (ib < 0) ib += str.size();
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

  class StringClass: public Command {
    //The function to check each character
    int (*is)(int);
    //Whether the range of is is restricted to 0..127
    bool asciiOnly;
    //Whether to negate the check
    bool negate;

    auto_ptr<Command> string;

    /* ISO C says that the char varieties are
     *   int f(int)
     * and the wchar_t are
     *   int f(wint_t).
     *
     * G++ considers the two incompatible (due to different signedness), but
     * that won't affect us. On most systems, sizeof(int) ==
     * sizeof(wint_t). Ensure that is true on this system.
     */
    void checkSizeofIntEqualsSizeofWintT() {
      switch (true) {
      case false:
      case sizeof(int) == sizeof(wint_t):;
      }
    }

  public:
    StringClass(Command* left,
                int (*is_)(int),
                bool asciiOnly_,
                bool negate_,
                auto_ptr<Command>& string_)
    : Command(left),
      is(is_), asciiOnly(asciiOnly_), negate(negate_),
      string(string_)
    { }

    virtual bool exec(wstring& out, Interpreter& interp) {
      wstring str;
      bool result;
      if (!interp.exec(str, string.get())) return false;

      if (str.empty()) {
        result = false;
        goto done;
      }

      for (unsigned i = 0; i < str.size(); ++i) {
        wchar_t curr = str[i];
        if ((!negate) ^
            ((!asciiOnly || (curr >= 0 && curr < 128)) &&
             is(curr))) {
          //Character failed
          result = false;
          goto done;
        }
      }

      //All chars passed
      result = true;

      done:
      out = (result? L"1" : L"0");
      return true;
    }
  };

  class StringClassParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const wstring& text, unsigned& offset) {
      wchar_t clazzch;
      unsigned clazzOffset;
      //The wchar_t functions take a wint_t. An assertion in StringClass
      //ensures that the functions are compatible.
      union {
        int (*a)(int);
        int (*u)(wint_t);
      } clazz;
      bool negate, asciiOnly;
      auto_ptr<Command> string;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.h(clazzch) >> clazzOffset, a.a(string)])
        return ParseError;

      switch (clazzch) {
      case L'l': clazz.u = iswlower, negate = false, asciiOnly = false; break;
      case L'L': clazz.u = iswlower, negate = true , asciiOnly = false; break;
      case L'o': clazz.a = islower , negate = false, asciiOnly = true ; break;
      case L'O': clazz.a = islower , negate = true , asciiOnly = true ; break;
      case L'u': clazz.u = iswupper, negate = false, asciiOnly = false; break;
      case L'U': clazz.u = iswupper, negate = true , asciiOnly = false; break;
      case L'v': clazz.a = isupper , negate = false, asciiOnly = true ; break;
      case L'V': clazz.a = isupper , negate = true , asciiOnly = true ; break;
      case L'a': clazz.u = iswalpha, negate = false, asciiOnly = false; break;
      case L'A': clazz.u = iswalpha, negate = true , asciiOnly = false; break;
      case L'b': clazz.a = isalpha , negate = false, asciiOnly = true ; break;
      case L'B': clazz.a = isalpha , negate = true , asciiOnly = true ; break;
      case L'n': clazz.u = iswalnum, negate = false, asciiOnly = false; break;
      case L'N': clazz.u = iswalnum, negate = true , asciiOnly = false; break;
      case L'm': clazz.a = isalnum , negate = false, asciiOnly = true ; break;
      case L'M': clazz.a = isalnum , negate = true , asciiOnly = true ; break;
      case L'\\':clazz.a = iscntrl , negate = false, asciiOnly = true ; break;
      case L'~': clazz.a = iscntrl , negate = true , asciiOnly = true ; break;
      case L'0': clazz.a = isdigit , negate = false, asciiOnly = true ; break;
      case L'9': clazz.a = isdigit , negate = true , asciiOnly = true ; break;
      case L'x': clazz.a = isxdigit, negate = false, asciiOnly = true ; break;
      case L'X': clazz.a = isxdigit, negate = true , asciiOnly = true ; break;
      case L'.':
      case L'p': clazz.u = iswpunct, negate = false, asciiOnly = false; break;
      case L':':
      case L'P': clazz.u = iswpunct, negate = true , asciiOnly = false; break;
      case L',':
      case L'q': clazz.a = ispunct , negate = false, asciiOnly = true ; break;
      case L';':
      case L'Q': clazz.a = ispunct , negate = true , asciiOnly = true ; break;
      case L's':
      case L'_': clazz.u = iswspace, negate = false, asciiOnly = false; break;
      case L'S':
      case L'#': clazz.u = iswspace, negate = true , asciiOnly = false; break;
      case L'g': clazz.u = iswgraph, negate = false, asciiOnly = false; break;
      case L'G': clazz.u = iswgraph, negate = true , asciiOnly = false; break;
      case L'h': clazz.a = isgraph , negate = false, asciiOnly = true ; break;
      case L'H': clazz.a = isgraph , negate = true , asciiOnly = true ; break;
      case L'r': clazz.u = iswprint, negate = false, asciiOnly = false; break;
      case L'R': clazz.u = iswprint, negate = true , asciiOnly = false; break;
      /* Not 's' since people might expect it to be space. */
      case L't': clazz.a = isprint , negate = false, asciiOnly = true ; break;
      case L'T': clazz.a = isprint , negate = true , asciiOnly = true ; break;

      default:
        interp.error(wstring(L"Unknown character class: ") + clazzch,
                     text, clazzOffset);
        return ParseError;
      }

      //OK
      out = new StringClass(out, clazz.a, asciiOnly, negate, string);
      return ContinueParsing;
    }
  };

  static GlobalBinding<StringClassParser> _stringClassParser(L"str-is");

  class StringLength: public UnaryCommand {
  public:
    StringLength(Command* left, auto_ptr<Command>& sub)
    : UnaryCommand(left, sub) {}

    virtual bool exec(wstring& dst, Interpreter& interp) {
      wstring s;
      if (!interp.exec(s, sub.get())) return false;

      dst = intToStr(s.size());
      return true;
    }
  };

  static GlobalBinding<UnaryCommandParser<StringLength> >
  _stringLengthParser(L"str-len");
}
