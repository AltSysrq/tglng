#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <map>
#include <cctype>
#include <iostream>

#include "interp.hxx"
#include "command.hxx"

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
}
