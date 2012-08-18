#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <map>

#include "fundamental.hxx"
#include "../command.hxx"
#include "../interp.hxx"
#include "../argument.hxx"
#include "../common.hxx"
#include "../function.hxx"

using namespace std;

namespace tglng {
  SelfInsertCommand::SelfInsertCommand(Command* left, wchar_t chr)
  : Command(left),
    value(1, chr)
  { }

  SelfInsertCommand::SelfInsertCommand(Command* left, const wstring& str)
  : Command(left),
    value(str)
  { }

  bool SelfInsertCommand::exec(wstring& dst, Interpreter&) {
    dst = value;
    return true;
  }

  ParseResult SelfInsertParser::parse(Interpreter&, Command*& out,
                                      const std::wstring& text,
                                      unsigned& offset) {
    out = new SelfInsertCommand(out, text[offset++]);
    return ContinueParsing;
  }

  static GlobalBinding<SelfInsertParser> _selfInsertParser(L"self-insert");

  class LongCommandParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const std::wstring& text,
                              unsigned& offset) {
      wstring name;
      unsigned nameStart;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.to(name, L'#') >> nameStart]) return ParseError;

      //Move back to the closing hash, since that is the effective character
      //for the command.
      --offset;

      map<wstring,CommandParser*>::const_iterator it =
        interp.commandsL.find(name);
      if (it == interp.commandsL.end()) {
        interp.error(wstring(L"No such command: ") + name, text, nameStart);
        return ParseError;
      }

      return it->second->parse(interp, out, text, offset);
    }
  };

  static GlobalBinding<LongCommandParser> _longCommandParser(L"long-command");

  class BindParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const std::wstring& text,
                              unsigned& offset) {
      wstring longName;
      wchar_t shortName;
      unsigned nameStart;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.to(longName, L'#') >> nameStart, a.h(shortName)])
        return ParseError;

      //Look the long command up
      map<wstring, CommandParser*>::const_iterator it =
        interp.commandsL.find(longName);
      if (it == interp.commandsL.end()) {
        interp.error(wstring(L"No such command: ") + longName, text, nameStart);
        return ParseError;
      }

      //Save the new binding, replacing anything that was there before.
      //Since commandsS doesn't own the CommandParser*s, we don't need to check
      //this.
      interp.commandsS[shortName] = it->second;
      //There is no actual command associated with bind; just leave out alone.
      return ContinueParsing;
    }
  };

  static GlobalBinding<BindParser> _bindParser(L"bind");

  ParseResult NullParser::parse(Interpreter& interp,
                                Command*&,
                                const wstring&,
                                unsigned& offset) {
    ++offset;
    return ContinueParsing;
  }

  static bool nullFunction(wstring* out, const wstring*,
                           Interpreter&, unsigned) {
    out[0].clear();
    return true;
  }

  bool NullParser::function(Function& fun) const {
    fun = Function(1,0, nullFunction);
    return true;
  }

  static GlobalBinding<NullParser> _nullParser(L"no-op");

  class MetaParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp,
                              Command*& out,
                              const wstring& text,
                              unsigned& offset) {
      ++offset; //Past command char
      out = new SelfInsertCommand(out, interp.escape);
      return ContinueParsing;
    }
  };

  static GlobalBinding<MetaParser> _metaParser(L"meta");

  class SetMetaParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp,
                              Command*& out,
                              const wstring& text,
                              unsigned& offset) {
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.h(interp.escape)]) return ParseError;
      return ContinueParsing;
    }
  };

  static GlobalBinding<SetMetaParser> _setMetaParser(L"set-meta");
}
