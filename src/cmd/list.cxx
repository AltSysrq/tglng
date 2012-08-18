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

  bool list::append(wstring* out, const wstring* in,
                    Interpreter& interp, unsigned) {
    if (!escape(out, in+1, interp, 0)) return false;
    if (!in[0].empty())
      out[0] = in[0] + L' ' + out[0];
    return true;
  }

  bool list::car(wstring* out, const wstring* in,
                 Interpreter& interp, unsigned silent) {
    wstring preout[2], prein[2];
    prein[0] = in[0];
    prein[1] = L"e";
    if (!defaultTokeniserPreprocessor(preout, prein,
                                      interp, 0))
      return false;

    if (preout[0].empty()) {
      if (!silent)
        wcerr << L"tglng: error: list-car: empty list" << endl;
      return false;
    }

    prein[0] = preout[0];
    return defaultTokeniser(out, prein, interp, 0);
  }

  bool list::map(wstring* out, const wstring* in,
                 Interpreter& interp, unsigned) {
    Function fun;
    if (!Function::get(fun, interp,
                       in[0], 1, 1))
      return false;

    out[0].clear();

    wstring carout[2];
    wstring remainder(in[1]);
    while (car(carout, &remainder, interp, 1)) {
      wstring appin[2];
      if (!fun.exec(appin+1, carout, interp, fun.parm))
        return false;

      appin[0] = out[0];
      remainder = carout[1];
      if (!append(out, appin, interp, 0))
        return false;
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

    wstring carout[2];
    wstring remainder(in[1]);
    while (car(carout, &remainder, interp, 1)) {
      wstring funin[2];
      funin[0] = carout[0];
      funin[1] = out[0];
      if (!fun.exec(out, funin, interp, fun.parm))
        return false;

      remainder = carout[1];
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
    wstring carout[2];
    wstring remainder(in[1]);
    while (car(carout, &remainder, interp, 1)) {
      wstring appin[2];
      if (!fun.exec(appin+1, carout, interp, fun.parm))
        return false;

      if (parseBool(appin[1])) {
        appin[1] = carout[0];
        appin[0] = out[0];
        if (!append(out, appin, interp, 0))
          return false;
      }

      remainder = carout[1];
    }

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
}
