#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <cctype>
#include <memory>
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
      wcerr << "tglng: error: compiling ";
      rx.showWhy();
      return false;
    }

    rx.input(in[1]);
    if (!rx.match()) {
      if (!rx) {
        wcerr << "tglng: error: executing ";
        rx.showWhy();
        return false;
      }

      //Not an error
      out[0] = L"0";
      out[1].clear();
      out[2].clear();
      out[3].clear();
      return true;
    }

    out[0] = L"1";
    out[1].clear();
    rx.tail(out[2]);
    rx.head(out[3]);
    //Build list of groups
    unsigned numGroups = rx.groupCount();
    for (unsigned i = 0; i < numGroups; ++i) {
      wstring group;
      rx.group(group, i);
      list::lappend(out[1], group);
    }

    return true;
  }

  static GlobalBinding<TFunctionParser<4,3,rxMatch> >
  _rxMatch(L"rx-match");

  class RegexMatchInline: public Command {
    auto_ptr<Regex> rx;
    auto_ptr<Command> sub;

  public:
    RegexMatchInline(Command* left,
                     auto_ptr<Regex>& rx_,
                     auto_ptr<Command>& sub_)
    : Command(left), rx(rx_), sub(sub_)
    { }

    virtual bool exec(wstring& dst, Interpreter& interp) {
      if (!*rx) return false; //Can't do anything with a broken regex

      wstring str;
      if (!interp.exec(str, sub.get()))
        return false;

      rx->input(str);
      if (!rx->match()) {
        if (!*rx) {
          wcerr << "tglng: error: executing ";
          rx->showWhy();
          return false;
        }

        //Just a failed match
        dst = L"0";
        return true;
      }

      //Match successful
      dst = L"1";
      unsigned numGroups = rx->groupCount();
      for (unsigned i = 0; i < 10; ++i)
        if (i < numGroups)
          rx->group(interp.registers[i + L'0'], i);
        else
          interp.registers[i + L'0'] = L"";
      rx->head(interp.registers[L'<']);
      rx->tail(interp.registers[L'>']);
      return true;
    }
  };

  class RegexMatchInlineParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp,
                              Command*& out,
                              const wstring& text,
                              unsigned& offset) {
      wstring options, pattern;
      auto_ptr<Command> sub;
      unsigned patternOffset;
      wchar_t delimiter;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), -a.an(options), a.h(delimiter),
             a.to(pattern, delimiter) >> patternOffset,
             a.a(sub)])
        return ParseError;

      auto_ptr<Regex> rx(new Regex(pattern, options));
      if (!*rx) {
        interp.error(L"Failed to compile regular expression.",
                     text, patternOffset);
        wcerr << L"tglng: error: compiling ";
        rx->showWhy();
        return ParseError;
      }

      out = new RegexMatchInline(out, rx, sub);
      return ContinueParsing;
    }
  };

  static GlobalBinding<RegexMatchInlineParser>
  _rxMatchInline(L"rx-match-inline");
}
