#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <map>
#include <cstdlib>

#include "fundamental.hxx"
#include "../command.hxx"
#include "../interp.hxx"
#include "../argument.hxx"
#include "../common.hxx"
#include "../function.hxx"
#include "basic_parsers.hxx"

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

      if (it->second->isTemporary) {
        interp.error(wstring(L"Command cannot be bound: ") + longName,
                     text, nameStart);
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

  class SetLocaleParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp,
                              Command*& out,
                              const wstring& text,
                              unsigned& offset) {
      wstring wlocaleName;
      unsigned nameOffset;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.to(wlocaleName, L'#') >> nameOffset])
        return ParseError;

      //All known non-EBCDIC systems use ASCII-only locale names, so transcode
      //the easy way
      string nlocaleName;
      nlocaleName.resize(wlocaleName.size());
      for (unsigned i = 0; i < wlocaleName.size(); ++i)
        nlocaleName[i] = (char)wlocaleName[i];

      setlocale(LC_ALL, nlocaleName.c_str());
      setlocale(LC_NUMERIC, "C");
      try {
        locale::global(locale(nlocaleName.c_str()));
      } catch (...) {
        interp.error(wstring(L"Failed to set locale to: ") + wlocaleName,
                     text, nameOffset);
        return ParseError;
      }
      return ContinueParsing;
    }
  };

  static GlobalBinding<SetLocaleParser> _setLocaleParser(L"set-locale");

  class Ignore: public UnaryCommand {
  public:
    Ignore(Command* left, auto_ptr<Command>& sub)
    : UnaryCommand(left, sub) {}

    virtual bool exec(wstring& dst, Interpreter& interp) {
      wstring ignored;
      if (!interp.exec(ignored, sub.get()))
        return false;

      dst.clear();
      return true;
    }
  };

  static GlobalBinding<UnaryCommandParser<Ignore> > _ignore(L"ignore");

  namespace {
    bool error(wstring*, const wstring* in, Interpreter&, unsigned) {
      wcerr << L"tglng: error: user: " << *in << endl;
      return false;
    }

    bool warn(wstring* out, const wstring* in, Interpreter&, unsigned) {
      wcerr << L"tglng: warn: user: " << *in << endl;
      out->clear();
      return true;
    }

    bool character(wstring* out, const wstring* in,
                   Interpreter& interp, unsigned) {
      signed code;
      if (!parseInteger(code, *in)) {
        wcerr << L"Invalid integer for character: " << *in << endl;
        return false;
      }

      out->assign(1, (wchar_t)code);
      return true;
    }

    bool characterCode(wstring* out, const wstring* in,
                       Interpreter& interp, unsigned) {
      if (in->empty()) {
        wcerr << L"Empty string to character-code" << endl;
        return false;
      }

      *out = intToStr((signed)(*in)[0]);
      return true;
    }

    static GlobalBinding<TFunctionParser<1,1,error> > _error(L"error");
    static GlobalBinding<TFunctionParser<1,1,warn > > _warn (L"warn" );
    static GlobalBinding<TFunctionParser<1,1,character> >
    _character(L"character");
    static GlobalBinding<TFunctionParser<1,1,characterCode> >
    _characterCode(L"character-code");
  }

  class Eval: public UnaryCommand {
  public:
    Eval(Command* left, auto_ptr<Command>& sub)
    : UnaryCommand(left, sub) {}

    virtual bool exec(wstring& dst, Interpreter& interp) {
      wstring code;
      if (!interp.exec(code, sub.get())) return false;

      auto_ptr<Command> dynamic;
      Command* out = NULL;
      unsigned offset = 0;
      switch (interp.parseAll(out, code, offset,
                              Interpreter::ParseModeCommand)) {
      case StopEndOfInput:
        break; //OK

      case StopCloseParen:
      case StopCloseBracket:
      case StopCloseBrace:
        interp.error(L"Unexpected closing parenthesis.", code, offset);
        //Fall through

      case ParseError:
        wcerr << L"Parsing dynamic code failed." << endl;
        delete out;
        return false;

      case ContinueParsing:
      default:
        //Should never happen
        wcerr << __FILE__ << L":" << __LINE__ << L": "
              << "Unexpected result from interp.parseAll" << endl;
        abort();
      }

      dynamic.reset(out);
      return interp.exec(dst, dynamic.get());
    }
  };

  static GlobalBinding<UnaryCommandParser<Eval> > _eval(L"eval");
}
