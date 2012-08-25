#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <cstring>
#include <iostream>
#include <vector>

//Include available regex headers
#ifdef HAVE_PRCRE_H
#include <pcre.h>
#endif
#ifdef HAVE_REGEX_H
#include <regex.h>
#endif

#include "regex.hxx"

//Determine support level.
//First check for specific requests from the configuration.
#if defined(FORCE_REGEX_PCRE16)
#  if defined(HAVE_PCRE_H) && defined(PCRE_SUPPORTS_16_BIT)
#    define TGLNG_REGEX_LEVEL TGLNG_REGEX_PCRE16
#  elif !defined(HAVE_PCRE_H)
#    error Configuration forces REGEX_PCRE16, but you do not have PCRE
#  else
#    error Configuration forces REGEX_PCRE16, but 16-bit not supported
#  endif
#elif defined(FORCE_REGEX_PCRE8)
#  if defined(HAVE_PCRE_H)
#    define TGLNG_REGEX_LEVEL TGLNG_REGEX_PCRE8
#  else
#    error Configuration forces REGEX_PCRE8, but you do not have PCRE
#  endif
#elif defined(FORCE_REGEX_POSIX)
#  if defined(HAVE_REGEX_H)
#    define TGLNG_REGEX_LEVEL TGLNG_REGEX_POSIX
#  else
#    error Configuration forces REGEX_POSIX, but your system does not have it
#  endif
#elif defined(FORCE_REGEX_NONE)
#  define TGLNG_REGEX_LEVEL TGLNG_REGEX_NONE
//Not forced, determine automatically
#elif defined(HAVE_PCRE_H) && defined(PCRE_SUPPORTS_16_BIT)
#  define TGLNG_REGEX_LEVEL TGLNG_REGEX_PCRE16
#elif defined(HAVE_PCRE_H)
#  define TGLNG_REGEX_LEVEL TGLNG_REGEX_PCRE8
#elif defined(HAVE_REGEX_H)
#  define TGLNG_REGEX_LEVEL TGLNG_REGEX_POSIX
#else
#  define TGLNG_REGEX_LEVEL TGLNG_REGEX_NONE
#endif

using namespace std;

namespace tglng {
  const unsigned regexLevel = TGLNG_REGEX_LEVEL;
#if TGLNG_REGEX_LEVEL == TGLNG_REGEX_NONE
  const wstring regexLevelName(L"NONE");
#elif TGLNG_REGEX_LEVEL == TGLNG_REGEX_POSIX
  const wstring regexLevelName(L"POSIX");
#elif TGLNG_REGEX_LEVEL == TGLNG_REGEX_PCRE8
  const wstring regexLevelName(L"PCRE8");
#elif TGLNG_REGEX_LEVEL == TGLNG_REGEX_PCRE16
  const wstring regexLevelName(L"PCRE16");
#endif

  //Functions to convert natvie wstrings to the type needed by the backend.
#if TGLNG_REGEX_LEVEL == TGLNG_REGEX_PCRE16
  typedef vector<PCRE_UCHAR16> rstring;
  static void convertString(rstring& dst, const wstring& src) {
    dst.resize(src.size()+1);
    for (unsigned i = 0; i < src.size(); ++i)
      if (src[i] <= 0xFFFF)
        dst[i] = src[i];
      else
        dst[i] = 0x001A;

    dst[src.size()] = 0;
  }
#elif TGLNG_REGEX_LEVEL == TGLNG_REGEX_PCRE8 || \
      TGLNG_REGEX_LEVEL == TGLNG_REGEX_POSIX
  typedef vector<char> rstring;
  static void convertString(rstring& dst, const wstring& src) {
    dst.resize(src.size()+1);
    for (unsigned i = 0; i < src.size(); ++i)
      if (src[i] <= 0xFF)
        dst[i] = (src[i] & 0xFF);
      else
        dst[i] = 0x1A;
    dst[src.size()] = 0;
  }
#endif

#if TGLNG_REGEX_LEVEL == TGLNG_REGEX_NONE
  //Null implementation; always fails at everything
  Regex::Regex(const wstring&, const wstring&)
  : data(*(RegexData*)NULL) {}
  Regex::~Regex() {}
  Regex::operator bool() const { return false; }
  void Regex::showWhy() const {
    wcerr << L"Regular expressions not supported in this build." << endl;
  }
  void Regex::input(const wstring&) {}
  bool Regex::match() { return false; }
  unsigned Regex::groupCount() const { return 0; }
  void Regex::group(wstring&, unsigned) const {}
  void Regex::tail(wstring&) const {}
#endif /* NONE */

#if TGLNG_REGEX_LEVEL == TGLNG_REGEX_POSIX
  //POSIX.1-2001 implementation
  struct RegexData {
    regex_t rx;
    int status; //0=OK, others=error
    rstring input;
    wstring rawInput;
    unsigned inputOffset;
    string why;
    regmatch_t matches[10];
  };

  Regex::Regex(const wstring& pattern, const wstring& options)
  : data(*new RegexData)
  {
    rstring rpattern;
    convertString(rpattern, pattern);

    int flags = REG_EXTENDED;
    //Parse options
    for (unsigned i = 0; i < options.size(); ++i)
      switch (options[i]) {
      case L'i':
        flags |= REG_ICASE;
        break;

      case L'l':
        flags |= REG_NEWLINE;
        break;
      }

    data.status = regcomp(&data.rx, &rpattern[0], flags);
    if (data.status) {
      //Save error message
      data.why.resize(regerror(data.status, &data.rx, NULL, 0));
      regerror(data.status, &data.rx, &data.why[0], data.why.size());
      //Free the pattern now, since the destructor won't think it exists
      regfree(&data.rx);
    }
  }

  Regex::~Regex() {
    if (data.status == 0)
      regfree(&data.rx);
    delete &data;
  }

  Regex::operator bool() const {
    return !data.status;
  }

  void Regex::showWhy() const {
    cerr << "POSIX extended regular expression: "
         << &data.why[0] << endl;
  }

  void Regex::input(const std::wstring& str) {
    convertString(data.input, str);
    data.inputOffset = 0;
    data.rawInput = str;
  }

  bool Regex::match() {
    data.status = regexec(&data.rx, &data.input[data.inputOffset],
                          sizeof(data.matches)/sizeof(data.matches[0]),
                          data.matches, 0);
    if (data.status) {
      //Failed for some reason
      data.why.resize(regerror(data.status, &data.rx, NULL, 0));
      regerror(data.status, &data.rx, &data.why[0], data.why.size());
      return false;
    }

    //Matched if the zeroth group is not -1
    if (-1 != data.matches[0].rm_eo)
      data.inputOffset = data.matches[0].rm_eo;
    return -1 != data.matches[0].rm_eo;
  }

  unsigned Regex::groupCount() const {
    unsigned cnt = 0;
    while (cnt < sizeof(data.matches)/sizeof(data.matches[0]) &&
           -1 != data.matches[cnt].rm_so)
      ++cnt;
    return cnt;
  }

  void Regex::group(wstring& out, unsigned ix) const {
    out.assign(data.rawInput, data.matches[ix].rm_so,
               data.matches[ix].rm_eo - data.matches[ix].rm_so);
  }

  void Regex::tail(wstring& out) const {
    out.assign(data.rawInput, data.inputOffset,
               data.rawInput.size() - data.inputOffset);
  }
#endif /* POSIX */
}
