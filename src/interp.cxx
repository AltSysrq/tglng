#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <map>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <iomanip>

#include "interp.hxx"
#include "command.hxx"
#include "common.hxx"

using namespace std;

namespace tglng {
  // This is a pointer since global bindings may be set up before this
  // compilation unit's initialisers run.
  static map<wstring,CommandParser*>* globalDefaultBindings = NULL;

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
    if (globalDefaultBindings) {
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
  : commandsL(cloneProxyBindings(globalDefaultBindings)),
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

  ParseResult Interpreter::parseAll(Command*& out,
                                    const wstring& text,
                                    unsigned& offset,
                                    ParseMode mode) {
    if (!out) {
      error(L"Don't know how to create no-op Command yet.", text, offset);
      return ParseError;
    }

    ParseResult result;
    while (!(result = parse(out, text, offset, mode)));

    return result;
  }

  bool Interpreter::exec(wstring& out, Command* cmd) {
    return cmd->exec(out, *this);
  }

  bool Interpreter::exec(wstring& out, const wstring& text, ParseMode mode) {
    Command* root = NULL;
    unsigned offset = 0;
    switch (parseAll(root, text, offset, mode)) {
    case ContinueParsing: //Shouldn't happen
    case StopEndOfInput:
      break; //OK

    case StopCloseParen:
    case StopCloseBracket:
    case StopCloseBrace:
      error(L"Unexpected closing parentheses, bracket, or brace.",
            text, offset);
      return false;

    case ParseError:
      return false;
    }

    return exec(out, root);
  }

  bool Interpreter::exec(wstring& out, wistream& in, ParseMode mode) {
    wstring text;

    //Read all text to EOF (either real or UNIX)
    getline(in, text, L'\4');

    if (in.fail()) {
      cerr << "Error reading input stream: " << strerror(errno) << endl;
      return false;
    }

    return exec(out, text, mode);
  }

  void Interpreter::error(const wstring& why,
                          const wstring& what, unsigned where) {
    wcerr << L"tglng: error: " << why << endl;
    //Grab up to 16 characters of context on either side
    unsigned contextStart = (where > 16? where - 16 : 0);
    unsigned contextEnd = (where + 16 < what.size()? where + 16 : what.size());
    wstring context = what.substr(contextStart, contextEnd);
    //Transliterate all whitespace to normal spaces
    for (unsigned i = 0; i < context.size(); ++i)
      if (iswspace(context[i]))
        context[i] = ' ';
    wcerr << L"  " << context << endl;
    wcerr << setw(2+contextStart+1) << L"^" << endl;
  }

  void Interpreter::bindGlobal(const wstring& name, CommandParser* parser) {
    //Initialise global list if this hasn't happened
    static bool hasInit = false;
    if (!hasInit) {
      hasInit = true;
      globalDefaultBindings = new map<wstring,CommandParser*>;
    }

    (*globalDefaultBindings)[name] = parser;
  }
}
