#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <map>
#include <cctype>
#include <cstdlib>
#include <iostream>

#include "interp.hxx"
#include "command.hxx"
#include "common.hxx"

using namespace std;

namespace tglng {
  // This is a pointer since global bindings may be set up before this
  // compilation unit's initialisers run.
  static map<wstring,CommandParser*>* globalDefaulBindings = NULL;

  // Proxies to another CommandParser which it does not own.
  class ProxyCommandParser: public CommandParser {
    CommandParser*const delegate;

  public:
    ProxyCommandParser(CommandParser* p) : delegate(p) {}
    virtual ParseResult parse(Interpreter& interp, Command*& accum,
                              const wstring& text, unsigned& offset) {
      return delegate->parse(interp, accum, text, offset);
    }
  };

  static map<wstring,CommandParser*> cloneProxyBindings(
    const map<wstring,CommandParser*>* defaults
  ) {
    map<wstring,CommandParser*> ret;
    if (globalDefaulBindings) {
      for (map<wstring,CommandParser*>::const_iterator it = defaults->begin();
           it != defaults->end(); ++it)
        ret[it->first] = new ProxyCommandParser(it->second);
    }
    return ret;
  }

  static map<wchar_t,CommandParser*> makeDefaultCommandsS(
    const map<wstring,CommandParser*>& commandsL
  ) {
    map<wchar_t,CommandParser*> ret;
    ret[L'#'] = commandsL.find(L"long-command")->second;
    return ret;
  }

  Interpreter::Interpreter()
  : commandsL(cloneProxyBindings(globalDefaulBindings)),
    commandsS(makeDefaultCommandsS(commandsL)),
    escape(L'`')
  {
  }

  Interpreter::Interpreter(const Interpreter* that)
  : commandsL(cloneProxyBindings(&that->commandsL)),
    //Since commandsS doesn't own anything anyway, a direct copy will suffice.
    commandsS(that->commandsS),
    escape(that->escape)
  { }

  Interpreter::~Interpreter() {
    //Delete the CommandParser*s owned by this.
    for (map<wstring,CommandParser*>::const_iterator it = commandsL.begin();
         it != commandsL.end(); ++it)
      delete it->second;
  }

  ParseResult Interpreter::parse(Command*& out,
                                 const wstring& text,
                                 unsigned& offset,
                                 ParseMode mode) {
    if (offset >= text.size())
      return StopEndOfInput;

    switch (mode) {
    case ParseModeLiteral:
      if (text[offset] != escape) {
    case ParseModeVerbatim:
        error(L"Don't know how to create self-insert command yet.",
              text, offset);
        return ParseError;
      } else {
        ++offset;
    case ParseModeCommand:
        //Skip leading whitespace
        while (offset < text.size() && iswspace(text[offset]))
          ++offset;

        //If we hit the end, it's OK in command mode but an error in literal
        //mode (since this came right after an escape).
        if (offset >= text.size()) {
          if (mode == ParseModeCommand)
            return StopEndOfInput;
          else {
            error(L"Expected command after escape character.", text, offset);
            return ParseError;
          }
        }

        CommandParser* parser;
        if (text[offset] == escape) {
          error(L"Don't know how to create self-insert command yet.",
                text, offset);
          return ParseError;
        } else {
          map<wchar_t,CommandParser*>::const_iterator it =
            commandsS.find(text[offset]);
          if (it == commandsS.end()) {
            error(L"Unknown command.", text, offset);
            return ParseError;
          }

          parser = it->second;
        }

        return parser->parse(*this, out, text, offset);
      }
    }

    //Shouldn't get here
    cerr << "FATAL: Unexpected value of mode to Interpreter::parse()." << endl;
    exit(EXIT_THE_SKY_IS_FALLING);
  }
}
