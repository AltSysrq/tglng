#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <cctype>
#include <vector>
#include <map>
#include <set>

#include "../command.hxx"
#include "../function.hxx"
#include "../interp.hxx"
#include "../argument.hxx"

using namespace std;

namespace tglng {
  /** Defines the possible options for the default tokeniser. */
  struct DefaultTokeniserOptions {
    /**
     * Whether spaces (via iswspace) are considered delimiters.
     * Default: true
     */
    bool spacesAreDelims;
    /**
     * Whether line feeds (\n, \r\n, or \r) are considered delimiters.
     * Default: false
     */
    bool linesAreDelims;
    /**
     * Whether NUL characters are considered delimiters.
     * Default: false
     */
    bool nulsAreDelims;
    /**
     * Additional characters to consider as delimiters.
     * Default: empty
     */
    set<wchar_t> additionalDelimiters;
    /**
     * If true, consecutive delimiters are treated as one delimiter. For
     * example, if comma were the only delimiter, the strings
     *   foo,,,bar
     *   foo,bar
     * would equivalently describe two tokens, "foo" and "bar"; but if this
     * setting is false, the former describes "foo", "", "", "bar".
     * Default: true
     */
    bool coalesceDelims;

    /**
     * Maps pairs of characters which must be balanced before a delimiter
     * counts. The closing character is always checked before the opening, so
     * they may be the same (eg, quote marks). Counting is only performed on
     * the outermost parenthesis.
     * Default: (), [], {}
     */
    map<wchar_t,wchar_t> parentheses;
    /**
     * Maps pairs of characters, which, if they enclose a string (taking
     * balancing into account, as in the parenthesis field), are stripped.
     * Ex:
     *   "(  foo )" -> "  foo "
     *   "(foo)bar(baz)" -> "(foo)bar(baz)"
     * Default: (), [], {}
     */
    map<wchar_t,wchar_t> trimParentheses;

    /**
     * If true, C-style backlash escape sequences will be processed. The
     * following are supported:
     *   \\     \
     *   \a     BEL
     *   \b     BS
     *   \e     ESC
     *   \f     FF
     *   \n     LF
     *   \r     CR
     *   \t     HT
     *   \v     VT
     *   \octal The Unicode character indicated by the following octal digits.
     *   \x##   The hex character ##
     *   \X##   Same as \x##
     *   \u#### The Unicode character #### (hex)
     *   \U######## The unicode character ########
     *   \u{...}
     *   \U{...} Same as \u or \U, but with any number of hexits within the
     *           braces.
     *   Anything else: The character being escaped
     *
     * Note that substitution is performed AFTER tokenisation and trimming. The
     * tokenisation/trim step is only aware of escape sequences enough to know
     * to ignore the character after the backslash.
     *
     * Default: false
     */
    bool escapeSequences;

    /**
     * Parses options from the given string.
     *
     * Each option starts with an optional '+' or '-' ('+' implied if omitted)
     * followed by an alphanumeric character indicating the option. '+'
     * indicates to set or add the option, and '-' to unset or delete it.
     *
     * The ! character will reset all options to the defaults, and _ will clear
     * all options.
     *
     * Each option may be followed by zero, one, or two other characters,
     * depending on the option, which affect exactly what it does.
     * The options are:
     *   s      Whether space characters are considered delimiters.
     *   l      Whether newlines are considered delimiters.
     *   n      Whether NUL characters are considered delimiters.
     *   dA     Understand the character A as a delimiter.
     *   S      Equivalent to +D+s-l-n+c. "-S" is meaningless.
     *   L      Equivalent to +D-s+l-n-c. "-L" is meaningless.
     *   0      Equivalent to _+n. "-0" is meaningless.
     *   D      Clears all delimiters set by d. "-D" is meaningless.
     *   c      Whether consecutive delimiters are coalesced.
     *   bAB    Treat A and B as parentheses, requiring them to be balanced
     *          before honouring delimiters. Implicitly sets "-tAB" if negative.
     *   tAB    If the characters A and B form a balanced pair around a token,
     *          strip them. Implicitly sets "+bAB" if positive.
     *   e      Whether backslash escape sequences are understood.
     *
     * If a "#" is encountered, characters are read to the next "#". The string
     * "tokfmt-" is prepended to the result, which is then looked up as a
     * command. This command must be a (1 <- 0) function. It is executed, and
     * its result is recursively parsed for more options.
     */
    DefaultTokeniserOptions(const wstring&, Interpreter& interp);

    /**
     * Resets this DefaultTokeniserOptions to the default settings.
     */
    void setDefaults();

    /**
     * Sets all boolean options to false, and clears all sets and maps.
     */
    void nuke();

    /**
     * Parses the given string, applying option adjustments as indicated within
     * the string.
     *
     * @param str The string to parse
     * @param interp The Interpreter to use for command lookup and execution.
     */
    void parse(const wstring& str, Interpreter& interp);
  };

  DefaultTokeniserOptions::DefaultTokeniserOptions(const wstring& str,
                                                   Interpreter& interp) {
    setDefaults();
    parse(str, interp);
  }

  void DefaultTokeniserOptions::setDefaults() {
    nuke();
    spacesAreDelims = true;
    coalesceDelims = true;
    static const wstring parens(L"()[]{}");
    for (unsigned i = 0; i < parens.size(); i += 2) {
      parentheses[parens[i]] = parens[i+1];
      trimParentheses[parens[i]] = parens[i+1];
    }
  }

  void DefaultTokeniserOptions::nuke() {
    spacesAreDelims = linesAreDelims = nulsAreDelims =
      coalesceDelims = escapeSequences = false;
    additionalDelimiters.clear();
    parentheses.clear();
    trimParentheses.clear();
  }

  void DefaultTokeniserOptions::parse(const wstring& str, Interpreter& interp) {
    bool positive = true;
    for (unsigned i = 0; i < str.size(); ++i) {
      switch (str[i]) {
      case L'+': positive = true; break;
      case L'-': positive = false; break;
      case L's':
        spacesAreDelims = positive;
        break;

      case L'l':
        linesAreDelims = positive;
        break;

      case L'n':
        nulsAreDelims = positive;
        break;

      case L'c':
        coalesceDelims = positive;
        break;

      case L'e':
        escapeSequences = positive;
        break;

      case L'_':
        nuke();
        break;

      case L'!':
        setDefaults();
        break;

      case L'd':
        if (++i < str.size()) {
          if (positive) additionalDelimiters.insert(str[i]);
          else          additionalDelimiters.erase (str[i]);
        }
        break;

      case L'S':
        spacesAreDelims = true;
        linesAreDelims = nulsAreDelims = false;
        additionalDelimiters.clear();
        coalesceDelims = true;
        break;

      case L'L':
        linesAreDelims = true;
        spacesAreDelims = nulsAreDelims = false;
        additionalDelimiters.clear();
        coalesceDelims = false;
        break;

      case L'0':
        nuke();
        nulsAreDelims = true;
        break;

      case L'b':
        if ((i += 2) < str.size()) {
          wchar_t l = str[i-1], r = str[i];
          if (positive)
            parentheses[l] = r;
          else {
            parentheses.erase(l);
            trimParentheses.erase(l);
          }
        }
        break;

      case L't':
        if ((i += 2) < str.size()) {
          wchar_t l = str[i-1], r = str[i];
          if (positive) {
            parentheses[l] = r;
            trimParentheses[l] = r;
          } else
            trimParentheses.erase(l);
        }

        break;

      case L'#': {
        unsigned len;
        ++i;
        for (len = 0; i+len < str.size() && str[i+len] != '#'; ++len);

        wstring cmdname(str, i, len);
        i += len;

        //Prepend prefix
        cmdname = L"tokfmt-" + cmdname;

        //Lookup and execute the command if possible
        if (interp.commandsL.count(cmdname)) {
          CommandParser* parser = interp.commandsL[cmdname];
          Function f;
          if (parser->function(f) && f.matches(1,0)) {
            wstring out;
            if (f.exec(&out, NULL, interp, f.parm)) {
              parse(out, interp);
            }
          }
        }
      } break;
      }

      //Reset positive if we didn't just make it false
      if (str[i] != L'-') positive = true;
    }
  }

  /**
   * Returns whether the given character is considered a delimiter according to
   * the given options.
   *
   * Note that, if true is returned, the caller MUST check whether:
   *   opts.linesAreDelims
   *   ch == L'\r'
   *   the character after ch == L'\n'
   * and, if those conditions are true, skip the next character.
   */
  static bool isdelim(wchar_t ch, const DefaultTokeniserOptions& opts) {
    return
      (opts.spacesAreDelims && iswspace(ch)) ||
      (opts.linesAreDelims && (ch == L'\n' || ch == L'\r')) ||
      (opts.nulsAreDelims && !ch) ||
      opts.additionalDelimiters.count(ch);
  }

  /**
   * Default tokeniser preprocessor.
   *
   * Iff options.coalesceDelims is true, skip all delimiters in the input
   * string.
   */
  bool defaultTokeniserPreprocessor(wstring* out, const wstring* in,
                                    Interpreter& interp, unsigned) {
    const wstring& str(in[0]);
    DefaultTokeniserOptions opts(in[1], interp);

    unsigned off = 0;
    if (opts.coalesceDelims)
      while (off < str.size() && isdelim(str[off], opts))
        ++off;

    out[0].assign(str, off, wstring::npos);
    return true;
  }

  /**
   * Default tokeniser implementation.
   */
  bool defaultTokeniser(wstring* out, const wstring* in,
                        Interpreter& interp, unsigned) {
    const wstring& str(in[0]);
    DefaultTokeniserOptions opts(in[1], interp);

    unsigned off;

    for (off = 0; off < str.size() && !isdelim(str[off], opts); ++off) {
      if (opts.escapeSequences && str[off] == L'\\') {
        //Ignore the next character
        ++off;
      } else if (opts.parentheses.count(str[off])) {
        //Balance the parens
        wchar_t l = str[off], r = opts.parentheses[str[off]];
        ++off;
        for (unsigned count = 1; count && off < str.size(); count && ++off)
          if      (str[off] == r) --count;
          else if (str[off] == l) ++count;

        //The above ends on the closing character, which is correct since the
        //outer loop update will advance beyond it.
      }
    }

    out[0].assign(str, 0, off /* excludes the delimiter we hit */);

    //Move past the delimiter if we didn't hit the end of the string
    if (off < str.size()) {
      //We know str[off] is a delimiter
      ++off;
      //But handle \r\n
      if (opts.linesAreDelims && off < str.size() &&
          str[off-1] == L'\r' && str[off] == L'\n')
        ++off;

      //Coalesce any further delimeters.
      //We don't need to check for \r\n here, since both of them are delimiters
      //anyway in line mode.
      if (opts.coalesceDelims)
        while (off < str.size() && isdelim(str[off], opts))
          ++off;
    }

    //Set the new remainder
    if (off < str.size())
      out[1].assign(str, off, wstring::npos);
    else
      out[1].clear();

    //Trim parens from the token if requested
    if (out[0].size() >= 2 && opts.trimParentheses.count(out[0][0])) {
      wchar_t l = out[0][0], r = opts.trimParentheses[out[0][0]];

      unsigned count, i;
      for (count = i = 1; i < out[0].size() && count; ++i)
        if      (out[0][i] == r) --count;
        else if (out[0][i] == l) ++count;

      //Trim if perfectly balanced; that is, if count == 0 and i == the length
      //of the string (in which case it was the final character which balanced
      //the initial).
      if (count == 0 && i == out[0].size())
        out[0] = out[0].substr(1, out[0].size()-2);
    }

    //Backslash substitution
    if (opts.escapeSequences) {
      wstring s;
      size_t ix = 0, next;
      while (ix < out[0].size()) {
        next = out[0].find(L'\\', ix);
        s.append(out[0], ix, next-ix);
        ix = next;
        if (ix == wstring::npos) break;

        //Move past backslash and handle whatever follows
        if (++ix < out[0].size()) {
          switch (out[0][ix]) {
          case L'a': s += L'\a'; break;
          case L'b': s += L'\b'; break;
          case L'e': s += L'\033'; break;
          case L'f': s += L'\f'; break;
          case L'n': s += L'\n'; break;
          case L'r': s += L'\r'; break;
          case L't': s += L'\t'; break;
          case L'v': s += L'\v'; break;

          case L'0':
          case L'1':
          case L'2':
          case L'3':
          case L'4':
          case L'5':
          case L'6':
          case L'7': {
            //Octal sequence
            wchar_t ch = 0;
            while (ix < out[0].size() &&
                   (out[0][ix] >= L'0' && out[0][ix] <= L'7')) {
              ch *= 8;
              ch += out[0][ix++] - L'0';
            }

            s += ch;
          } break;

          case L'x':
          case L'X':
          case L'u':
          case L'U': {
            unsigned fixedLen =
              (out[0][ix] == L'x'? 2 :
               out[0][ix] == L'X'? 2 :
               out[0][ix] == L'u'? 4 :
               /*         == L'U'*/8);
            wchar_t ch = 0;

            ++ix;
            if (ix < out[0].size()) {
              if (out[0][ix] == L'{') {
                ++ix;
                //Enclosed sequence
                while (ix < out[0].size() && iswxdigit(out[0][ix])) {
                  ch *= 16;
                  if (out[0][ix] >= L'0' && out[0][ix] <= L'9')
                    ch += out[0][ix] - L'0';
                  else if (out[0][ix] <= 'A' && out[0][ix] <= 'F')
                    ch += out[0][ix] - L'A' + 10;
                  else /* (out[0][ix] <= 'a' && out[0][ix] <= 'f') */
                    ch += out[0][ix] - L'a' + 10;
                  ++ix;
                }
                //Move past closing brace
                if (ix < out[0].size() && out[0][ix] == L'}')
                  ++ix;
              } else {
                //Exact count
                while (ix < out[0].size() && iswxdigit(out[0][ix]) &&
                       fixedLen--) {
                  ch *= 16;
                  if (out[0][ix] >= L'0' && out[0][ix] <= L'9')
                    ch += out[0][ix] - L'0';
                  else if (out[0][ix] <= 'A' && out[0][ix] <= 'F')
                    ch += out[0][ix] - L'A' + 10;
                  else /* (out[0][ix] <= 'a' && out[0][ix] <= 'f') */
                    ch += out[0][ix] - L'a' + 10;
                  ++ix;
                }
              }
            }

            s += ch;
          } break;

          default:
            s += out[0][ix++];
          } //end switch(wchar_t)
        } //end if (is backslash),
      } //end for (each character)

      out[0] = s;
    } //end if (escape sequences)

    return true;
  }

  static GlobalBinding<TFunctionParser<2,2,defaultTokeniserPreprocessor> >
  _defaultTokeniserPre(L"default-tokeniser-pre");
  static GlobalBinding<TFunctionParser<2,2,defaultTokeniser> >
  _defaultTokeniser(L"default-tokeniser");
}
