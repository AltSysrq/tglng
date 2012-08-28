#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>
#include <cerrno>
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>

#include "../command.hxx"
#include "../function.hxx"
#include "../interp.hxx"
#include "../common.hxx"

using namespace std;

namespace tglng {
  bool cmdGetenv(wstring* out, const wstring* in,
                 Interpreter& interp, unsigned) {
    vector<char> name;
    if (!wstrtontbs(name, *in)) {
      wcerr << L"Could not convert to narrow string for getenv: "
            << *in << endl;
      return false;
    }
    const char* value = getenv(&name[0]);
    if (value) {
      if (!ntbstowstr(out[0], value)) {
        wcerr << L"Could not widen string in getenv: "
              << value << endl;
        return false;
      }
      out[1] = L"1";
    } else {
      out[0].clear();
      out[1] = L"0";
    }
    return true;
  }

  bool cmdSetenv(wstring* out, const wstring* in,
                 Interpreter& interp, unsigned) {
    vector<char> name, value;
    if (!wstrtontbs(name, in[0]) ||
        !wstrtontbs(value, in[1])) {
      wcerr << L"Could not convert to narrow string for setenv: "
            << in[0] << endl << L"or: " << in[1] << endl;
      return false;
    }

    if (setenv(&name[0], &name[1], true)) {
      wcerr << L"Could not set " << in[0] << L" to " << in[1]
            << L": " << strerror(errno) << endl;
      return false;
    }

    out->clear();
    return true;
  }

  static GlobalBinding<TFunctionParser<2,1,cmdGetenv> >
  _getenv(L"getenv");
  static GlobalBinding<TFunctionParser<1,2,cmdSetenv> >
  _setenv(L"setenv");
}
