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
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.to(name, L'#')]) return ParseError;

      //Move back to the closing hash, since that is the effective character
      //for the command.
      --offset;

      map<wstring,CommandParser*>::const_iterator it =
        interp.commandsL.find(name);
      if (it == interp.commandsL.end()) {
        interp.error(wstring(L"Unknown command: ") + name, text, offset);
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
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.to(longName, L'#'), a.h(shortName)])
        return ParseError;

      //Look the long command up
      map<wstring, CommandParser*>::const_iterator it =
        interp.commandsL.find(longName);
      if (it == interp.commandsL.end()) {
        interp.error(wstring(L"Unknown command: ") + longName, text, offset-2);
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
}
