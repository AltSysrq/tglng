#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <cctype>

#include "list.hxx"
#include "../interp.hxx"
#include "../command.hxx"
#include "../argument.hxx"
#include "../function.hxx"
#include "../tokeniser.hxx"
#include "../common.hxx"
#include "default_tokeniser.hxx"

using namespace std;

namespace tglng {
  bool list::escape(wstring* out, const wstring* in,
                    Interpreter& interp, unsigned) {
    wstring str(in[0]);

    //See what characters of concern are present
    bool
      hasSpace = false,
      hasParen = false,
      hasBrack = false,
      hasBrace = false,
      hasSlash = false;
    for (unsigned i = 0; i < str.size(); ++i) {
      hasSpace |= iswspace(str[i]);
      hasParen |= str[i] == L'(' || str[i] == L')';
      hasBrack |= str[i] == L'[' || str[i] == L']';
      hasBrace |= str[i] == L'{' || str[i] == L'}';
      hasSlash |= str[i] == L'\\';
    }

    /* The string must be enclosed in a parenthesis-like pair only if it
     * contains spaces or any other parentheses.
     * Try to use a paren which is not present in the string. If all types are
     * used, use braces and escape the others.
     */
    bool escapeBraces = hasParen && hasBrack && hasBrace;

    //Escape backslashes, and braces if needed
    if (hasSlash || escapeBraces) {
      for (unsigned i = 0; i < str.size(); ++i) {
        if (str[i] == L'\\')
          str.insert(i++, 1, L'\\');
        else if ((str[i] == L'{' || str[i] == L'}') &&
                 escapeBraces)
          str.insert(i++, 1, L'\\');
      }
    }

    if (hasSpace || hasParen || hasBrack || hasBrace) {
      if (!hasParen)
        str = L'(' + str + L')';
      else if (!hasBrack)
        str = L'[' + str + L']';
      else
        str = L'{' + str + L'}';
    }

    out[0] = str;
    return true;
  }

  void list::lappend(wstring& list, const wstring& item) {
    if (list.empty())
      escape(&list, &item, *(Interpreter*)NULL, 0);
    else {
      wstring escaped;
      escape(&escaped, &item, *(Interpreter*)NULL, 0);
      list += L' ';
      list += escaped;
    }
  }

  bool list::append(wstring* out, const wstring* in,
                    Interpreter& interp, unsigned) {
    if (!escape(out, in+1, interp, 0)) return false;
    if (!in[0].empty())
      out[0] = in[0] + L' ' + out[0];
    return true;
  }

  bool list::lcar(wstring& car, wstring& cdr,
                  const wstring& list, Interpreter& interp) {
    wstring preout[2], prein[2];
    prein[0] = list;
    prein[1] = L"e";
    if (!defaultTokeniserPreprocessor(preout, prein,
                                      interp, 0))
      return false;

    if (preout[0].empty()) {
      return false;
    }

    prein[0] = preout[0];

    if (!defaultTokeniser(preout, prein, interp, 0))
      return false;

    car = preout[0];
    cdr = preout[1];
    return true;
  }

  bool list::car(wstring* out, const wstring* in,
                 Interpreter& interp, unsigned silent) {
    if (lcar(out[0], out[1], in[1], interp))
      return true;
    else {
      if (!silent)
        wcerr << L"tglng: error: list-car: empty list" << endl;
      return false;
    }
  }

  bool list::map(wstring* out, const wstring* in,
                 Interpreter& interp, unsigned) {
    Function fun;
    if (!Function::get(fun, interp,
                       in[0], 1, 1))
      return false;

    out[0].clear();

    wstring remainder(in[1]), item;
    while (lcar(item, remainder, remainder, interp)) {
      wstring result;
      if (!fun.exec(&result, &item, interp, fun.parm))
        return false;

      lappend(out[0], result);
    }

    return true;
  }

  bool list::fold(wstring* out, const wstring* in,
                  Interpreter& interp, unsigned) {
    Function fun;
    if (!Function::get(fun, interp,
                       in[0], 1, 2))
      return false;

    out[0] = in[2];

    wstring remainder(in[1]), item;
    while (lcar(item, remainder, remainder, interp)) {
      wstring funin[2];
      funin[0] = item;
      funin[1] = out[0];
      if (!fun.exec(out, funin, interp, fun.parm))
        return false;
    }

    return true;
  }

  bool list::filter(wstring* out, const wstring* in,
                    Interpreter& interp, unsigned) {
    Function fun;
    if (!Function::get(fun, interp,
                       in[0], 1, 1))
      return false;

    out[0].clear();
    wstring remainder(in[1]), item;
    while (lcar(item, remainder, remainder, interp)) {
      wstring result;
      if (!fun.exec(&result, &item, interp, fun.parm))
        return false;

      if (parseBool(result))
        lappend(out[0], item);
    }

    return true;
  }

  unsigned list::llength(const wstring& list, Interpreter& interp) {
    wstring remainder(list), item;
    unsigned len = 0;
    while (lcar(item, remainder, remainder, interp)) {
      ++len;
    }

    return len;
  }

  bool list::length(wstring* out, const wstring* in,
                    Interpreter& interp, unsigned) {
    out[0] = intToStr(llength(in[0], interp));
    return true;
  }

  bool list::ix(wstring* out, const wstring* in,
                Interpreter& interp, unsigned) {
    signed ix;
    if (!parseInteger(ix, in[1])) {
      wcerr << L"Invalid integer for list index: " << in[1] << endl;
      return false;
    }

    if (ix < 0) {
      unsigned len = llength(in[0], interp);
      ix += len;

      if (ix < 0) {
        wcerr << L"Integer out of range for list index: "
              << in[1] << L" (list length is " << len << L")" << endl;
        return false;
      }
    }

    //We must loop one additional time since the first call returns the zeroth
    //item.
    ++ix;

    wstring remainder(in[0]);
    unsigned len = 0;
    while (ix && lcar(out[0], remainder, remainder, interp)) {
      --ix, ++len;
    }

    if (ix) {
      wcerr << L"Integer out of range for list index: "
            << in[1] << L" (list length is " << len << L")" << endl;
      return false;
    }

    return true;
  }

  bool list::zip(wstring* out, const wstring* in,
                 Interpreter& interp, unsigned) {
    vector<wstring> lists;
    wstring item, remainder(in[0]);

    while (lcar(item, remainder, remainder, interp))
      lists.push_back(item);

    out[0].clear();
    bool someListIsNonEmpty;
    do {
      someListIsNonEmpty = false;

      for (unsigned i = 0; i < lists.size(); ++i) {
        if (lcar(item, lists[i], lists[i], interp))
          lappend(out[0], item);

        someListIsNonEmpty |= !lists[i].empty();
      }
    } while (someListIsNonEmpty);

    return true;
  }

  bool list::flatten(wstring* out, const wstring* in,
                     Interpreter& interp, unsigned) {
    wstring list, lists(in[0]);
    out[0].clear();
    while (lcar(list, lists, lists, interp)) {
      if (out[0].empty())
        out[0] = list;
      else {
        out[0] += L' ';
        out[0] += list;
      }
    }

    return true;
  }

  bool list::unzip(wstring* out, const wstring* in,
                   Interpreter& interp, unsigned) {
    signed stride = 2;
    wstring item, list(in[0]);
    if (!in[1].empty()) {
      if (!parseInteger(stride, in[1]) || stride <= 1) {
        wcerr << L"tglng: error: Invalid integer for list-unzip stride: "
              << in[1] << endl;
        return false;
      }
    }

    vector<wstring> lists(stride);
    while (!list.empty()) {
      for (unsigned i = 0; i < lists.size() && !list.empty(); ++i) {
        if (lcar(item, list, list, interp))
          lappend(lists[i], item);
      }
    }

    out[0].clear();
    for (unsigned i = 0; i < lists.size(); ++i)
      lappend(out[0], lists[i]);

    return true;
  }

  static GlobalBinding<TFunctionParser<2,1,list::car> >
  _listCar(L"list-car");
  static GlobalBinding<TFunctionParser<1,1,list::escape> >
  _listEscape(L"list-escape");
  static GlobalBinding<TFunctionParser<1,2,list::append> >
  _listAppend(L"list-append");
  static GlobalBinding<TFunctionParser<1,2,list::map> >
  _listMap(L"list-map");
  static GlobalBinding<TFunctionParser<1,3,list::fold> >
  _listFold(L"list-fold");
  static GlobalBinding<TFunctionParser<1,2,list::filter> >
  _listFilter(L"list-filter");
  static GlobalBinding<TFunctionParser<1,1,list::length> >
  _listLength(L"list-length");
  static GlobalBinding<TFunctionParser<1,2,list::ix> >
  _listIndex(L"list-ix");
  static GlobalBinding<TFunctionParser<1,1,list::zip> >
  _listZip(L"list-zip");
  static GlobalBinding<TFunctionParser<1,1,list::flatten> >
  _listFlatten(L"list-flatten");
  static GlobalBinding<TFunctionParser<1,2,list::unzip> >
  _listUnzip(L"list-unzip");

  class ListAssign: public Command {
    wstring registers;
    AutoSection sub;

  public:
    ListAssign(Command* left,
               const wstring& registers_,
               const Section& sub_)
    : Command(left), registers(registers_), sub(sub_)
    { }

    virtual bool exec(wstring& dst, Interpreter& interp) {
      wstring item, list;
      if (!sub.exec(list, interp))
        return false;

      for (unsigned i = 0; i < registers.size() &&
             list::lcar(item, list, list, interp); ++i)
        interp.registers[registers[i]] = item;

      dst = list;
      return true;
    }
  };

  class ListAssignParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const wstring& text, unsigned& offset) {
      wstring registers;
      AutoSection sub;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.ns(registers), a.s(sub)])
        return ParseError;;

      out = new ListAssign(out, registers, sub);
      sub.clear();
      return ContinueParsing;
    }
  };

  static GlobalBinding<ListAssignParser> _listAssign(L"list-assign");

  class ListConvert: public Command {
    Tokeniser tokeniser;
    AutoSection sub;

  public:
    ListConvert(Command* left,
                const Tokeniser& tokeniser_,
                const Section& sub_)
    : Command(left), tokeniser(tokeniser_), sub(sub_)
    { }

    virtual bool exec(wstring& dst, Interpreter& interp) {
      wstring item, list;
      if (!sub.exec(list, interp))
        return false;

      tokeniser.reset(list);

      dst.clear();
      while (tokeniser.next(item))
        list::lappend(dst, item);

      return !tokeniser.error();
    }
  };

  class ListConvertParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const wstring& text, unsigned& offset) {
      Function init, next;
      wstring sinit(L"default-tokeniser-pre"), snext(L"default-tokeniser"),
        options;
      unsigned initOff(0), nextOff(0);
      AutoSection sub;
      bool prependPlus(false), prependMinus(false);

      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(),
             -(a.x(L'%'), a.to(sinit, L'%') >> initOff),
             -(a.x(L'#'), a.to(snext, L'#') >> nextOff),
             -((a.x(prependPlus, L'+') | a.x(prependMinus, L'-')),
               a.ns(options)),
             a.s(sub)])
        return ParseError;

      if (!Function::get(init, interp, sinit, 2, 2,
                         text, initOff, &Function::compatible))
        return ParseError;
      if (!Function::get(next, interp, snext, 2, 2,
                         text, nextOff, &Function::matches))
        return ParseError;

      if      (prependPlus)  options.insert(0,1,L'+');
      else if (prependMinus) options.insert(0,1,L'-');
      out = new ListConvert(out,
                            Tokeniser(interp, init, next, L"", options),
                            sub);
      sub.clear();
      return ContinueParsing;
    }
  };

  static GlobalBinding<ListConvertParser> _listConvert(L"list-convert");
}
