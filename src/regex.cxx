#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

//Include available regex headers
#ifdef HAVE_PCRE_H
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
    wcerr << L"regular expressions not supported in this build." << endl;
  }
  void Regex::input(const wstring&) {}
  bool Regex::match() { return false; }
  unsigned Regex::groupCount() const { return 0; }
  void Regex::group(wstring&, unsigned) const {}
  void Regex::tail(wstring&) const {}
  void Regex::head(wstring&) const {}
  unsigned Regex::where() const { return 0; }
#endif /* NONE */

#if TGLNG_REGEX_LEVEL == TGLNG_REGEX_POSIX
  //POSIX.1-2001 implementation
  struct RegexData {
    regex_t rx;
    int status; //0=OK, others=error
    rstring input;
    wstring rawInput;
    unsigned inputOffset, headBegin, headEnd;
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
    wcerr << "POSIX extended regular expression: "
          << &data.why[0] << endl;
  }

  void Regex::input(const std::wstring& str) {
    convertString(data.input, str);
    data.inputOffset = 0;
    data.rawInput = str;
  }

  bool Regex::match() {
    //Manually fail the empty string
    if (data.inputOffset >= data.input.size()) {
      data.status = 0;
      memset(data.matches, -1, sizeof(data.matches));
      return false;
    }

    data.status = regexec(&data.rx, &data.input[data.inputOffset],
                          sizeof(data.matches)/sizeof(data.matches[0]),
                          data.matches, 0);
    if (data.status == REG_NOMATCH) {
      //Transform to a more uniform result
      //(Nothing went wrong, no error should be returned, in theory)
      data.status = 0;
      memset(data.matches, -1, sizeof(data.matches));
    }
    if (data.status) {
      //Failed for some reason
      data.why.resize(regerror(data.status, &data.rx, NULL, 0));
      regerror(data.status, &data.rx, &data.why[0], data.why.size());
      //Free the pattern now, since the destructor won't think it exists
      regfree(&data.rx);
      return false;
    }

    //Matched if the zeroth group is not -1
    if (-1 != data.matches[0].rm_eo) {
      //Update offsets
      data.headBegin = data.inputOffset;
      data.headEnd = data.matches[0].rm_so;
      data.inputOffset = data.matches[0].rm_eo;
    }
    return -1 != data.matches[0].rm_eo;
  }

  unsigned Regex::groupCount() const {
    //Elements in the middle may be unmatched if that particular group was
    //excluded, so search for the last group.
    unsigned last;
    for (unsigned i = 0; i < sizeof(data.matches)/sizeof(data.matches[0]); ++i)
      if (-1 != data.matches[i].rm_so)
        last = i;

    return last+1;
  }

  void Regex::group(wstring& dst, unsigned ix) const {
    //Indices may be negative if this group didn't match
    if (data.matches[ix].rm_so == -1)
      dst.clear();
    else
      dst.assign(data.rawInput, data.matches[ix].rm_so,
                 data.matches[ix].rm_eo - data.matches[ix].rm_so);
  }

  void Regex::tail(wstring& dst) const {
    dst.assign(data.rawInput, data.inputOffset,
               data.rawInput.size() - data.inputOffset);
  }

  void Regex::head(wstring& dst) const {
    dst.assign(data.rawInput, data.headBegin,
               data.headEnd - data.headBegin);
  }

  unsigned Regex::where() const {
    return 0;
  }
#endif /* POSIX */

#if TGLNG_REGEX_LEVEL == TGLNG_REGEX_PCRE8 ||   \
    TGLNG_REGEX_LEVEL == TGLNG_REGEX_PCRE16
#  if TGLNG_REGEX_LEVEL == TGLNG_REGEX_PCRE8
#    define pcreN pcre
#    define pcreN_compile    pcre_compile
#    define pcreN_exec       pcre_exec
#    define pcreN_free       pcre_free
#    define pcreN_maketables pcre_maketables
#  else
#    define pcreN pcre16
#    define pcreN_compile    pcre16_compile
#    define pcreN_exec       pcre16_exec
#    define pcreN_free       pcre16_free
#    define pcreN_maketables pcre16_maketables
#  endif

  /* By default, PCRE only classifies characters based on ASCII (some builds
   * may instead automatically rebuild the tables according to the "C" locale
   * (why not the system locale?), but we can't count on that.
   *
   * Whenever the current C locale (by setlocale(LC_ALL,NULL)) differs from the
   * lastPcreTableLocale, generate a new table.
   */
  static string lastPcreTableLocale("C");
  static const unsigned char* localPcreTable(NULL);

  struct RegexData {
    pcreN* rx;
    unsigned errorOffset;
    /* We want ten matches. Each match takes two entries. Additionally, PCRE
     * requires that we allocate an extra entry at the end for each match.
     */
#define MAX_MATCHES 10
    signed matches[MAX_MATCHES*3];
    rstring input;
    wstring rawInput;
    unsigned inputOffset, headBegin, headEnd;
    string errorMessage;
  };

  Regex::Regex(const wstring& pattern, const wstring& options)
  : data(*new RegexData)
  {
    rstring rpattern;
    const char* errorMessage = NULL;
    data.errorOffset = 0;
    convertString(rpattern, pattern);

    //Parse the options
    int flags = PCRE_DOTALL | PCRE_DOLLAR_ENDONLY;
    for (unsigned i = 0; i < options.size(); ++i)
      switch (options[i]) {
      case L'i':
        flags |= PCRE_CASELESS;
        break;

      case L'l':
        flags &= ~(PCRE_DOTALL | PCRE_DOLLAR_ENDONLY);
        //Would be nice if there were a constant for this
        //(Then again,
        //  flags &= ~(`[PCRE_NEWLINE_`E{CR LF CRLF ANYCRLF ANY}| |`]);
        flags &= ~(PCRE_NEWLINE_CR | PCRE_NEWLINE_LF | PCRE_NEWLINE_CRLF |
                   PCRE_NEWLINE_ANYCRLF | PCRE_NEWLINE_ANY);
        flags |= PCRE_MULTILINE | PCRE_NEWLINE_ANYCRLF;
        break;
      }

    //Generate a table if necessary
    if (lastPcreTableLocale != setlocale(LC_ALL, NULL)) {
      lastPcreTableLocale = setlocale(LC_ALL, NULL);
      if (localPcreTable)
        pcreN_free(const_cast<void*>(static_cast<const void*>(localPcreTable)));
      localPcreTable = pcreN_maketables();
    }

    data.rx = pcreN_compile(&rpattern[0], flags, &errorMessage,
                            (int*)&data.errorOffset, localPcreTable);
    if (!data.rx)
      data.errorMessage = errorMessage;
  }

  Regex::~Regex() {
    if (data.rx)
      pcreN_free(data.rx);
    delete &data;
  }

  Regex::operator bool() const {
    return !!data.rx;
  }

  void Regex::showWhy() const {
    wcerr << L"Perl-Compatible Regular Expression: "
          << data.errorMessage.c_str() << endl;
  }

  void Regex::input(const wstring& str) {
    convertString(data.input, str);
    data.rawInput = str;
    data.inputOffset = 0;
  }

  bool Regex::match() {
    //Set all elements to -1
    memset(data.matches, -1, sizeof(data.matches));
    int status = pcreN_exec(data.rx, NULL,
                            &data.input[0],
                            //Subtract one for term NUL
                            data.input.size() - 1,
                            data.inputOffset,
                            PCRE_NOTEMPTY,
                            data.matches,
                            sizeof(data.matches)/sizeof(data.matches[0]));
    if (status < 0) {
      //Error (there doesn't seem to be any way to get an error message)
      ostringstream msg;
      msg << "Perl-Compatible Regular Expression: error code " << status;
      data.errorMessage = msg.str();
      //Free the rx so that operator bool() returns false
      pcreN_free(data.rx);
      data.rx = NULL;
      return false;
    }

    //PCRE can return 0 even for successful matches (eg, there were more groups
    //than provided), so check for success manually.
    //Any group which was not matched will have entries of -1. If matching is
    //successful, group 0 is defined.
    if (data.matches[0] != -1) {
      //Advance input
      data.headBegin = data.inputOffset;
      data.headEnd = data.matches[0];
      data.inputOffset = data.matches[1];
      return true;
    } else {
      return false;
    }
  }

  unsigned Regex::groupCount() const {
    //PCRE groups are indexed by occurrance within the pattern, so some groups
    //might not match; find the last matched group.
    unsigned last = 0;
    for (unsigned i = 0; i < MAX_MATCHES; ++i)
      if (data.matches[i*2] != -1)
        last = i;

    return last+1;
  }

  void Regex::group(wstring& dst, unsigned ix) const {
    //Some middle groups may be unmatched, so return an empty string if -1
    if (data.matches[ix*2] == -1)
      dst.clear();
    else
      dst.assign(data.rawInput, data.matches[ix*2],
                 data.matches[ix*2+1] - data.matches[ix*2]);
  }

  void Regex::head(wstring& dst) const {
    dst.assign(data.rawInput, data.headBegin,
               data.headEnd - data.headBegin);
  }

  void Regex::tail(wstring& dst) const {
    dst.assign(data.rawInput, data.inputOffset,
               data.rawInput.size() - data.inputOffset);
  }

  unsigned Regex::where() const {
    return data.errorOffset;
  }
#endif /* PCRE* */
}
