#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <map>
#include <utility>

#include "../command.hxx"
#include "../interp.hxx"
#include "../argument.hxx"
#include "../common.hxx"

using namespace std;

namespace tglng {
  class Ensemble;
  typedef map<pair<Interpreter*,wstring>, Ensemble*> ensembles_t;
  static ensembles_t ensembles;

  class Ensemble: public CommandParser {
    Interpreter*const parentInterp;
    const wstring name;
    map<wchar_t, CommandParser*> commands;

  public:
    Ensemble(Interpreter* par, const wstring& name_)
    : parentInterp(par), name(name_) {
      ensembles.insert(make_pair(make_pair(par,name_), this));
    }

    virtual ~Ensemble() {
      ensembles.erase(make_pair(parentInterp, name));
    }

    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const wstring& text, unsigned& offset) {
      wchar_t subcommand;
      unsigned newOffset;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.h(subcommand) >> newOffset]) return ParseError;

      offset = newOffset; //Back up to the subcommand char itself

      map<wchar_t, CommandParser*>::const_iterator it =
        commands.find(subcommand);
      if (it == commands.end()) {
        interp.error(wstring(L"No such ensemble subcommand: ") + subcommand,
                     text, newOffset);
        return ParseError;
      }

      return it->second->parse(interp, out, text, offset);
    }

    void bind(wchar_t command, CommandParser* parser) {
      commands[command] = parser;
    }
  };

  class EnsembleNewParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const wstring& text,
                              unsigned& offset) {
      wstring name;
      unsigned nameOffset;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.to(name, L'#') >> nameOffset]) return ParseError;

      if (interp.commandsL.count(name)) {
        interp.error(wstring(L"Command name already in use: ") + name,
                     text, nameOffset);
        return ParseError;
      }

      interp.commandsL[name] = new Ensemble(&interp, name);
      return ContinueParsing;
    }
  };

  static GlobalBinding<EnsembleNewParser> _ensembleNewParser(L"ensemble-new");

  class EnsembleBindParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const wstring& text,
                              unsigned& offset) {
      wstring ename, cname;
      unsigned enameOffset, cnameOffset;
      wchar_t shortname;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(),
             a.to(ename, L'#') >> enameOffset,
             a.to(cname, L'#') >> cnameOffset,
             a.h(shortname)])
        return ParseError;

      ensembles_t::const_iterator eit =
        ensembles.find(make_pair(&interp, ename));
      if (eit == ensembles.end()) {
        interp.error(wstring(L"No such ensemble: ") + ename, text, enameOffset);
        return ParseError;
      }

      map<wstring,CommandParser*>::const_iterator cit =
        interp.commandsL.find(cname);
      if (cit == interp.commandsL.end()) {
        interp.error(wstring(L"No such command: ") + cname, text, cnameOffset);
        return ParseError;
      }

      eit->second->bind(shortname, cit->second);
      return ContinueParsing;
    }
  };

  static GlobalBinding<EnsembleBindParser>
  _ensembleBindParser(L"ensemble-bind");
}
