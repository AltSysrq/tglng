#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <cctype>

#include "../interp.hxx"
#include "../command.hxx"
#include "../argument.hxx"
#include "basic_parsers.hxx"

using namespace std;

namespace tglng {
  /* Magic case conversion is performed as a two-pass process.
   *
   * The first pass examines the input string and determines hinting properties
   * from it (ie, what cases or separators are present).
   *
   * The second transforms the string by repeadedly executing a function for
   * each character, given the hint. This function may also alter what function
   * is executed for the next character.
   */

  /* Indicates that one or more lowercase characters are present in the input
   * string.
   */
#define HINT_LC (1<<0)
  /* Indicates that one or more uppercase characters are present in the input
   * string.
   */
#define HINT_UC (1<<1)
  /* Indicates that both upper and lowercase characters are present in the
   * input string. This is automatically set if both LC and UC are set.
   */
#define HINT_MC (1<<2)
  /* Indicates that space-like delimiters (space characters, hyphens, or
   * underscores) are present in the input string.
   */
#define HINT_SEP (1<<4)

  /* Wrapper struct since it is otherwise impossible to define a function which
   * takes itself as a parm.
   */
  struct Converter {
    /**
     * Performs conversion of a single character.
     *
     * @param ch The single-character string to convert.
     * @param hint A bitwise OR of any number of the HINT constants.
     * @param next The next function to use, by default the one being called.
     * @return The transformed character.
     */
    typedef void (*f_t)(wstring& ch, unsigned hint, Converter& next);
    f_t f;
  };

  typedef wint_t (*case_t)(wint_t);

  static int isseparator(wint_t ch) {
    return iswspace(ch) || ch == L'_' || ch == L'-';
  }

  /* Basic to-uppercase or to-lower converter. */
  namespace { /* Since templates can't take functions with static linkage (?) */
  template<case_t F>
  void simpleConverter(wstring& ch, unsigned, Converter&) {
    ch[0] = F(ch[0]);
  }
  }

  /* Common functions for converting to delimited-word and mixed-case formats.
   *
   * If separators exist in the input string, they define the word boundaries
   * and may be replaced by the expected delimiter (see ReplaceSeparators);
   * other word characters are normalised according to WordInit and WordRest.
   *
   * If no separators exist, an uppercase character following a lowercase
   * character is considered a word boundary, and will cause a delimiter to be
   * inserted between the two.
   *
   * Digits after any character will start a word, and any alpha after a digit
   * will as well.
   *
   * A Delimiter of NUL means nothing.
   */
  template<wchar_t Delimiter, bool ReplaceSeparators,
           case_t TokenInit, case_t WordInit, case_t WordRest>
  class delimitedConverter {
    template<bool StartToken, bool StartWord,
             bool PUChar, bool PLChar, bool PDigit>
    static void main(wstring& ch, unsigned hint, Converter& next) {
      bool wasU = iswupper(ch[0]),
           wasD = iswdigit(ch[0]);
      if (wasD || iswalpha(ch[0])) {
        if (StartToken) {
          ch[0] = TokenInit(ch[0]);
        } else if ((PLChar && (wasU || wasD) && !(hint & HINT_SEP)) ||
                   (PDigit && !wasD && !(hint & HINT_SEP)) ||
                   StartWord) {
          //New word
          ch[0] = WordInit(ch[0]);
          if (Delimiter && !StartWord)
            //Insert our own word start
            ch.insert(0, 1, Delimiter);
        } else {
          //Other word char
          ch[0] = WordRest(ch[0]);
        }

        if (wasU)
          next.f = main<false,false,true,false,false>;
        else if (wasD)
          next.f = main<false,false,false,true,false>;
        else
          //If there are any letters which are neither upper nor lower,
          //consider them lower.
          next.f = main<false,false,false,true,false>;
      } else if (isseparator(ch[0])) {
        if (ReplaceSeparators) {
          if (Delimiter)
            ch[0] = Delimiter;
          else
            //NUL delimiter, remove the separator
            ch.clear();

          //Next char is word start
          next.f = main<false,true,false,false,false>;
        }
      } else {
        //Non-word non-separator
        //Next one is the start of a new token
        next.f = main<true,false,false,false,false>;
      }
    }

  public:
    static void f(wstring& ch, unsigned hint, Converter& next) {
      main<true,false,false,false,false>(ch, hint, next);
    }
  };

  template<Converter::f_t InitF>
  class MagicCaseConverter: public UnaryCommand {
  public:
    MagicCaseConverter(Command* left, auto_ptr<Command> sub)
    : UnaryCommand(left, sub) {}

    virtual bool exec(wstring& dst, Interpreter& interp) {
      wstring in;
      if (!interp.exec(in, sub.get())) return false;

      //Determine hints
      unsigned hint = 0;
      for (unsigned i = 0; i < in.size(); ++i) {
        if (iswlower(in[i])) hint |= HINT_LC;
        if (iswupper(in[i])) hint |= HINT_UC;
        if (isseparator(in[i])) hint |= HINT_SEP;
      }
      if ((hint & (HINT_LC|HINT_UC)) == (HINT_LC|HINT_UC))
        hint |= HINT_MC;

      //Convert
      dst.clear();
      Converter conv;
      conv.f = InitF;
      wstring ch;
      for (unsigned i = 0; i < in.size(); ++i) {
        ch.assign(1, in[i]);
        conv.f(ch, hint, conv);
        dst += ch;
      }

      //OK
      return true;
    }
  };

  static GlobalBinding<
    UnaryCommandParser<
      MagicCaseConverter<
        simpleConverter<
          towlower> > > > _strtolower(L"str-tolower");
  static GlobalBinding<
    UnaryCommandParser<
      MagicCaseConverter<
        simpleConverter<
          towupper> > > > _strtoupper(L"str-toupper");

  static GlobalBinding<
    UnaryCommandParser<
      MagicCaseConverter<
        delimitedConverter<L' ', false,
                           towupper, towupper, towlower>::f> > >
  _strtotitle(L"str-totitle");
  static GlobalBinding<
    UnaryCommandParser<
      MagicCaseConverter<
        delimitedConverter<L' ', false,
                           towupper, towlower, towlower>::f> > >
  _strtosent(L"str-tosent");
  static GlobalBinding<
    UnaryCommandParser<
      MagicCaseConverter<
        delimitedConverter<0, true,
                           towlower, towupper, towlower>::f> > >
  _strtocamel(L"str-tocamel");
  static GlobalBinding<
    UnaryCommandParser<
      MagicCaseConverter<
        delimitedConverter<0, true,
                           towupper, towupper, towlower>::f> > >
  _strtopascal(L"str-topascal");
  static GlobalBinding<
    UnaryCommandParser<
      MagicCaseConverter<
        delimitedConverter<L'_', true,
                           towupper, towupper, towupper>::f> > >
  _strtoscream(L"str-toscream");
  static GlobalBinding<
    UnaryCommandParser<
      MagicCaseConverter<
        delimitedConverter<L'_', true,
                           towlower, towlower, towlower>::f> > >
  _strtocstyle(L"str-tocstyle");
  static GlobalBinding<
    UnaryCommandParser<
      MagicCaseConverter<
        delimitedConverter<L'_', true,
                           towupper, towupper, towlower>::f> > >
  _strtocaspal(L"str-tocaspal");
  static GlobalBinding<
    UnaryCommandParser<
      MagicCaseConverter<
        delimitedConverter<L'-', true,
                           towlower, towlower, towlower>::f> > >
  _strtolisp(L"str-tolisp");
  static GlobalBinding<
    UnaryCommandParser<
      MagicCaseConverter<
        delimitedConverter<L'-', true,
                           towupper, towupper, towupper>::f> > >
  _strtocobol(L"str-tocobol");
}
