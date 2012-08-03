#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <cctype>
#include <iostream>

using namespace std;

namespace tglng {
  bool parseInteger(signed& dst, const wstring& str,
                    unsigned offset, unsigned* end) {
    unsigned ix = offset;
    bool negative = false;
    unsigned base = 10;

    dst = 0;

    //Skip leading whitespace
    while (ix < str.size() && iswspace(str[ix]))
      ++ix;

    //If hit EOI, reject
    if (ix >= str.size()) {
      if (end) *end = ix;
      return false;
    }

    //Leading sign
    if (str[ix] == L'+') ++ix;
    else if (str[ix] == L'-') {
      ++ix;
      negative = true;
    }

    if (ix >= str.size()) {
      if (end) *end = ix;
      return false;
    }

    //Leading base
    if (ix + 2 < str.size() && str[ix] == L'0') {
      switch (str[ix+1]) {
      case L'b':
      case L'B':
        base = 2;
        ix += 2;
        break;

      case L'o':
      case L'O':
        base = 8;
        ix += 2;
        break;

      case L'x':
      case L'X':
        base = 16;
        ix += 2;
        break;
      }
    }

    //Digits
    do {
      unsigned value;
      wchar_t curr = str[ix++];

      if (curr >= L'0' && curr <= L'9')
        value = curr - L'0';
      else if (curr >= L'a' && curr <= L'f')
        value = 10 + curr - L'a';
      else if (curr >= L'A' && curr <= L'F')
        value = 10 + curr - L'A';
      else
        break;

      if (value >= base)
        break;

      dst *= base;
      dst += value;
    } while (ix < str.size());

    //Trailing whitespace
    while (ix < str.size() && iswspace(str[ix]))
      ++ix;

    //Sign
    if (negative)
      dst = -dst;

    //If !end, we don't allow trailing garbage; otherwise, we do
    if (end) *end = ix;
    return ix == str.size() || end;
  }
}
