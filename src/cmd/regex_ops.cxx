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
      interp.error(L"Regular expression error here (maybe).",
                   in[1], rx.where());
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
        interp.error(L"Regular expression error here (maybe).",
                     text, patternOffset + rx->where());
        return ParseError;
      }

      out = new RegexMatchInline(out, rx, sub);
      return ContinueParsing;
    }
  };

  static GlobalBinding<RegexMatchInlineParser>
  _rxMatchInline(L"rx-match-inline");

  bool rxReplace(wstring* out, const wstring* in,
                 Interpreter& interp, unsigned) {
    out->clear();
    signed limit = 0x7FFFFFFF;
    if (!in[3].empty() && !parseInteger(limit, in[3])) {
      wcerr << L"Invalid integer for rx-repl limit: " << in[3] << endl;
      return false;
    }

    Regex rx(in[0], in[4]);
    if (!rx) {
      wcerr << L"tglng: error: compiling ";
      rx.showWhy();
      interp.error(L"Regular expression error here (maybe).",
                   in[0], rx.where());
      return false;
    }

    rx.input(in[2]);
    wstring tail(in[2]);
    while (limit-- && rx.match()) {
      wstring head;
      rx.head(head);
      *out += head;
      *out += in[1];
      rx.tail(tail);
    }

    if (!rx) {
      wcerr << L"tglng: error: executing ";
      rx.showWhy();
      return false;
    }

    *out += tail;
    return true;
  }

  static GlobalBinding<TFunctionParser<1,5,rxReplace> >
  _rxReplace(L"rx-repl");

  bool rxReplaceEach(wstring* out, const wstring* in,
                     Interpreter& interp, unsigned) {
    Function fun;
    signed limit = 0x7FFFFFFF;
    if (!in[3].empty() && !parseInteger(limit, in[3])) {
      wcerr << L"Invalid integer for rx-repl-each limit: " << in[3] << endl;
      return false;
    }

    if (!Function::get(fun, interp, in[1], 1, 2))
      return false;

    Regex rx(in[0], in[4]);
    if (!rx) {
      wcerr << L"tglng: error: compiling ";
      rx.showWhy();
      interp.error(L"Regular expression error here (maybe).",
                   in[0], rx.where());
      return false;
    }

    out->clear();
    rx.input(in[2]);
    wstring tail(in[2]);
    while (limit-- && rx.match()) {
      wstring parms[2], replacement;
      rx.group(parms[0], 0);
      unsigned ngroups = rx.groupCount();
      for (unsigned i = 0; i < ngroups; ++i) {
        wstring group;
        rx.group(group, i);
        list::lappend(parms[1], group);
      }

      if (!fun.exec(&replacement, parms, interp, fun.parm))
        return false;

      wstring head;
      rx.head(head);
      *out += head;
      *out += replacement;
      rx.tail(tail);
    }

    //Check for error
    if (!rx) {
      wcerr << L"tglng: error: executing ";
      rx.showWhy();
      return false;
    }

    *out += tail;
    return true;
  }

  static GlobalBinding<TFunctionParser<1,5,rxReplaceEach> >
  _rxReplaceEach(L"rx-repl-each");

  class RxReplaceInline: public Command {
    auto_ptr<Regex> rx;
    auto_ptr<Command> limit;
    AutoSection str, replacement;

  public:
    RxReplaceInline(Command* left,
                    auto_ptr<Regex>& rx_,
                    auto_ptr<Command>& limit_,
                    const Section& str_,
                    const Section& replacement_)
    : Command(left),
      rx(rx_),
      limit(limit_),
      str(str_),
      replacement(replacement_)
    { }

    virtual bool exec(wstring& dst, Interpreter& interp) {
      signed limit = 0x7FFFFFFF;
      if (this->limit.get()) {
        wstring slimit;
        if (!interp.exec(slimit, this->limit.get()))
          return false;
        if (!parseInteger(limit, slimit)) {
          wcerr << L"Invalid integer for rx-repl-inline limit: "
                << slimit << endl;
          return false;
        }
      }

      wstring str;
      if (!this->str.exec(str, interp))
        return false;

      rx->input(str);
      wstring tail(str);
      dst.clear();
      while (limit-- && rx->match()) {
        //Bind registers
        unsigned ngroups = rx->groupCount();
        for (unsigned i = 0; i < 10; ++i)
          if (i < ngroups)
            rx->group(interp.registers[L'0' + i], i);
          else
            interp.registers[L'0' + i].clear();

        wstring head;
        rx->head(head);
        rx->tail(tail);
        interp.registers[L'<'] = head;
        interp.registers[L'>'] = tail;

        //Run the section to get the replacement
        wstring replacement;
        if (!this->replacement.exec(replacement, interp))
          return false;

        dst += head;
        dst += replacement;
      }

      //Check for error
      if (!*rx) {
        wcerr << L"tglng: error: executing ";
        rx->showWhy();
        return false;
      }

      dst += tail;
      return true;
    }
  };

  class RxReplaceInlineParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const wstring& text, unsigned& offset) {
      wstring options, pattern;
      auto_ptr<Regex> rx;
      auto_ptr<Command> limit;
      AutoSection str, replacement;
      wchar_t delimiter;
      unsigned patternOffset;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), -a.an(options), a.h(delimiter),
             a.to(pattern, delimiter) >> patternOffset,
             a.a(limit) -= a.s(replacement), a.s(str)])
        return ParseError;

      rx.reset(new Regex(pattern, options));
      if (!*rx) {
        interp.error(L"Failed to compile regular expression.",
                     text, patternOffset);
        wcerr << L"tglng: error: compiling ";
        rx->showWhy();
        interp.error(L"Regular expression error here (maybe).",
                     text, patternOffset + rx->where());
        return ParseError;
      }

      out = new RxReplaceInline(out, rx, limit, str, replacement);
      str.clear();
      replacement.clear();
      return ContinueParsing;
    }
  };

  static GlobalBinding<RxReplaceInlineParser>
  _rxReplaceInline(L"rx-replace-inline");
}
