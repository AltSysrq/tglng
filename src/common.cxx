#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <cstring>
#include <cctype>
#include <iostream>
#include <sstream>
#include <locale>
#include <clocale>
#include <vector>

#include "common.hxx"

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
      wchar_t curr = str[ix];

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
      ++ix;
    } while (ix < str.size());

    //Trailing whitespace.
    //Don't skip if trailing garbage is allowed.
    while (ix < str.size() && iswspace(str[ix]) && !end)
      ++ix;

    //Sign
    if (negative)
      dst = -dst;

    //If !end, we don't allow trailing garbage; otherwise, we do
    if (end) *end = ix;
    return ix == str.size() || end;
  }

  bool parseBool(const wstring& str) {
    signed asNumber;
    if (parseInteger(asNumber, str))
      return asNumber != 0;
    else
      return str.size() > 0;
  }

  wstring intToStr(signed value) {
    wostringstream out;
    out.imbue(locale("C"));
    out << value;
    return out.str();
  }

  typedef codecvt<wchar_t, char, mbstate_t> converter_t;
  bool wstrtontbs(vector<char>& dst, const wstring& src, const char** endOut) {
    locale theLocale;
    const converter_t& converter =
      use_facet<converter_t>(theLocale);
    dst.resize(src.size() * converter.max_length() + 1);
    mbstate_t state = mbstate_t();
    const wchar_t* from_next;
    char* end;
    converter_t::result result =
      converter.out(state, src.data(), src.data() + src.size(),
                    from_next, &dst[0], &dst[0] + dst.size() - 1, end);

    if (result != converter_t::ok) return false;

    *end = 0;
    if (endOut)
      *endOut = end;
    return true;
  }

  bool wstrtostr(string& dst, const wstring& src) {
    vector<char> tmp;
    const char* end;
    if (!wstrtontbs(tmp, src, &end)) return false;

    dst.assign((const char*)&tmp[0], end);
    return true;
  }

  bool ntbstowstr(wstring& dst, const char* src, const char* send) {
    locale theLocale;
    const converter_t& converter =
      use_facet<converter_t>(theLocale);

    size_t len = send? send - src : strlen(src);
    vector<wchar_t> tmp(len);
    const char* from_next;
    wchar_t* end;
    //Failing to assign this crashes the UTF-8 converter.
    //Shouldn't the default constructor do this?
    //(Or is mbstate_t some primitive type which doesn't implicitly default
    //construct?)
    mbstate_t state = mbstate_t();
    converter_t::result result =
      converter.in(state, src, src + len, from_next,
                   &tmp[0], &tmp[0] + tmp.size(), end);

    if (result != converter_t::ok) return false;

    dst.assign(&tmp[0], end);
    return true;
  }

  bool strtowstr(wstring& dst, const string& src) {
    return ntbstowstr(dst, src.data(), src.data() + src.size());
  }
}
