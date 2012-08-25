#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <cctype>
#include <iostream>

#include "../command.hxx"
#include "../function.hxx"
#include "../argument.hxx"
#include "../interp.hxx"
#include "../regex.hxx"
#include "list.hxx"
#include "../common.hxx"

using namespace std;

namespace tglng {
  bool rxSupport(wstring* out, const wstring*, Interpreter&, unsigned) {
    *out = regexLevelName;
    return true;
  }

  static GlobalBinding<TFunctionParser<1,0,rxSupport> >
  _rxSupport(L"rx-support");

  bool rxMatch(wstring* out, const wstring* in,
               Interpreter& interp, unsigned) {
    Regex rx(in[0], in[2]);
    if (!rx) {
      cerr << "tglng: error: compiling ";
      rx.showWhy();
      return false;
    }

    rx.input(in[1]);
    if (!rx.match()) {
      if (!rx) {
        cerr << "tglng: error: executing ";
        rx.showWhy();
        return false;
      }

      //Not an error
      out[0] = L"0";
      out[1].clear();
      out[2].clear();
      return true;
    }

    out[0] = L"1";
    out[1].clear();
    rx.tail(out[2]);
    //Build list of groups
    unsigned numGroups = rx.groupCount();
    for (unsigned i = 0; i < numGroups; ++i) {
      wstring group;
      rx.group(group, i);
      list::lappend(out[1], group);
    }

    return true;
  }

  static GlobalBinding<TFunctionParser<3,3,rxMatch> >
  _rxMatch(L"rx-match");
}
