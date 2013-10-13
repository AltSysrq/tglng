#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <list>
#include <map>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <iomanip>

#include "interp.hxx"
#include "command.hxx"
#include "cmd/fundamental.hxx"
#include "cmd/long_mode.hxx"
#include "options.hxx"
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

    virtual bool function(Function& dst) const {
      return delegate->function(dst);
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
  : nextExternalEntity(0),
    commandsL(cloneProxyBindings(globalDefaultBindings)),
    commandsS(makeDefaultCommandsS(commandsL)),
    escape(L'`'), longMode(false)
  {
  }

  Interpreter::Interpreter(const Interpreter* that)
  : externalEntities(that->externalEntities),
    nextExternalEntity(that->nextExternalEntity),
    commandsL(cloneProxyBindings(&that->commandsL)),
    //Since commandsS doesn't own anything anyway, a direct copy will suffice.
    commandsS(that->commandsS),
    escape(that->escape), longMode(that->longMode)
  {
    //Clear the free fields of the externals since we don't own the objects
    for (map<unsigned,ExtrernalEntity>::iterator it = externalEntities.begin();
         it != externalEntities.end(); ++it)
      it->second.free = NULL;
  }

  Interpreter::~Interpreter() {
    //Free the externals
    for (map<unsigned,ExtrernalEntity>::const_iterator it =
           externalEntities.begin();
         it != externalEntities.end(); ++it)
      if (it->second.free)
        it->second.free(it->second.datum);

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

    backupDest = offset;

    switch (mode) {
    case ParseModeLiteral:
      if (text[offset] != escape) {
    case ParseModeVerbatim:
        out = new SelfInsertCommand(out, text[offset++]);
        return ContinueParsing;
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
          static SelfInsertParser selfInsertParser;
          static NullParser nullParser;
          if (mode == ParseModeLiteral)
            parser = &selfInsertParser;
          else
            parser = &nullParser;
        } else {
          if (longMode && LongModeCmdParser::isname(text[offset]) &&
              //We want - to be the subtraction operator, so names can't start
              //with it.
              text[offset] != L'-') {
            static LongModeCmdParser longModeCmdParser;
            parser = &longModeCmdParser;
          } else {
            map<wchar_t,CommandParser*>::const_iterator it =
              commandsS.find(text[offset]);
            if (it == commandsS.end()) {
              error(wstring(L"No such command: ") + text[offset], text, offset);
              return ParseError;
            }

            parser = it->second;
          }
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
    ParseResult result;
    while (!(result = parse(out, text, offset, mode)));

    return result;
  }

  void Interpreter::backup(unsigned& ix) const {
    ix = backupDest;
  }

  bool Interpreter::exec(wstring& out, Command* cmd) {
    //Reverse Command::left linked list so we don't need to recurse
    list<Command*> lhs;
    for (Command* curr = cmd; curr; curr = curr->left)
      lhs.push_front(curr);
    //Accumulate result
    out.clear();
    wstring result;
    for (list<Command*>::const_iterator it = lhs.begin();
         it != lhs.end(); ++it) {
      if (!(*it)->exec(result, *this))
        return false;
      out += result;
    }

    return true;
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
            text, offset-1 /* -1 for back to command char */);
      if (root) delete root;
      return false;

    case ParseError:
      if (root) delete root;
      return false;
    }

    bool res = exec(out, root);
    delete root;
    return res;
  }

  bool Interpreter::exec(wstring& out, wistream& in, ParseMode mode) {
    wstring text;

    //Read all text to EOF (either real or UNIX)
    getline(in, text, L'\4');

    if (in.fail() && !in.eof()) {
      cerr << "Error reading input stream: " << strerror(errno) << endl;
      return false;
    }

    return exec(out, text, mode);
  }

  unsigned Interpreter::bindExternal(const ExtrernalEntity& ext) {
    do {
      ++nextExternalEntity;
    } while (externalEntities.count(nextExternalEntity));

    externalEntities[nextExternalEntity] = ext;
    return nextExternalEntity;
  }

  void* Interpreter::external(unsigned ref) const {
    return externalEntities.find(ref)->second.datum;
  }

  void Interpreter::error(const wstring& why,
                          const wstring& what, unsigned where) {
    wcerr << L"tglng: error: " << why << endl;
    //Grab up to 16 characters of context on either side
    unsigned contextStart = (where > 16? where - 16 : 0);
    unsigned contextEnd = (where + 16 < what.size()? where + 16 : what.size());
    wstring context = what.substr(contextStart, contextEnd - contextStart);
    //Transliterate all whitespace to normal spaces
    for (unsigned i = 0; i < context.size(); ++i)
      if (iswspace(context[i]))
        context[i] = ' ';
    wcerr << L"  " << context << endl;
    wcerr << setw(2+where-contextStart+1) << L"^" << endl;

    if (locateParseError) {
      /* Unset, since we only print once */
      locateParseError = false;

      wcout << where << endl;
    }
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

  void Interpreter::freeGlobalBindings() {
    for (map<wstring,CommandParser*>::iterator it =
           globalDefaultBindings->begin();
         it != globalDefaultBindings->end(); ++it)
      delete it->second;
    delete globalDefaultBindings;
    globalDefaultBindings = NULL;
  }
}
