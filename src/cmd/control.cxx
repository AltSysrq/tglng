#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <memory>

#include "../command.hxx"
#include "../argument.hxx"
#include "../interp.hxx"
#include "../common.hxx"
#include "../tokeniser.hxx"

using namespace std;

namespace tglng {
  class If: public Command {
    AutoSection condition, then, otherwise;

  public:
    If(Command* left,
       const Section& c,
       const Section& t,
       const Section& o)
    : Command(left),
      condition(c), then(t), otherwise(o)
    { }

    virtual bool exec(wstring& dst, Interpreter& interp) {
      wstring cond;
      if (!condition.exec(cond, interp)) return false;
      return (parseBool(cond)? then : otherwise).exec(dst, interp);
    }
  };

  class IfParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const wstring& text, unsigned& offset) {
      AutoSection condition, then, otherwise;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.s(condition), a.s(then), -a.s(otherwise)])
        return ParseError;

      out = new If(out, condition, then, otherwise);
      condition.clear();
      then.clear();
      otherwise.clear();
      return ContinueParsing;
    }
  };

  static GlobalBinding<IfParser> _ifParser(L"if");

  class FalseCoalesce: public Command {
    AutoSection lhs, rhs;

  public:
    FalseCoalesce(Command* left,
                  const Section& l,
                  const Section& r)
    : Command(left), lhs(l), rhs(r) {}

    virtual bool exec(wstring& dst, Interpreter& interp) {
      if (!lhs.exec(dst, interp)) return false;
      return parseBool(dst) || rhs.exec(dst, interp);
    }
  };

  class FalseCoalesceParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const wstring& text, unsigned& offset) {
      AutoSection lhs, rhs;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.s(lhs), a.s(rhs)]) return ParseError;

      out = new FalseCoalesce(out, lhs, rhs);
      lhs.clear();
      rhs.clear();
      return ContinueParsing;
    }
  };

  static GlobalBinding<FalseCoalesceParser>
  _falseCoalesceParser(L"false-coalesce");

  class ForInteger: public Command {
    //Whether to emit the counter before each execution of the right part of
    //the body section.
    bool emitCounterImplicitly;
    //The register on which to operate
    wchar_t reg;
    //The limiting value and the increment
    auto_ptr<Command> init, limit, increment;
    AutoSection body;

  public:
    ForInteger(Command* left,
               bool emitCounterImplicitly_,
               wchar_t reg_,
               auto_ptr<Command>& init_,
               auto_ptr<Command>& limit_,
               auto_ptr<Command>& increment_,
               const Section& body_)
    : Command(left),
      emitCounterImplicitly(emitCounterImplicitly_),
      reg(reg_),
      init(init_),
      limit(limit_),
      increment(increment_),
      body(body_)
    { }

    virtual bool exec(wstring& dst, Interpreter& interp) {
      dst.clear();

      wstring str;
      signed slim, sinit, sinc;

      //Initialise the parms and register
      if (limit.get()) {
        if (!interp.exec(str, limit.get())) return false;
        if (!parseInteger(slim, str)) {
          wcerr << L"Invalid integer for for-integer limit: " << str << endl;
          return false;
        }
      } else {
        slim = 10;
      }

      if (init.get()) {
        if (!interp.exec(str, init.get())) return false;
        if (!parseInteger(sinit, str)) {
          wcerr << L"Invalid integer for for-integer init: " << str << endl;
          return false;
        }
        interp.registers[reg] = str;
      } else {
        sinit = 0;
        interp.registers[reg] = L"0";
      }

      if (increment.get()) {
        if (!interp.exec(str, increment.get())) return false;
        if (!parseInteger(sinc, str) || !sinc) {
          wcerr << L"Invalid integer for for-integer increment: "
                << str << endl;
          return false;
        }
      } else {
        if (sinit <= slim) sinc = +1;
        else               sinc = -1;
      }

      for (signed curr = sinit; sinc > 0? curr < slim : curr > slim;
           /* Increment performed in body */) {
        //Run the body parts
        if (!interp.exec(str, body.left)) return false;
        dst += str;
        if (emitCounterImplicitly) {
          map<wchar_t,wstring>::const_iterator it = interp.registers.find(reg);
          if (it == interp.registers.end()) {
            wcerr << L"for-integer loop register " << reg
                  << L" was unset during execution." << endl;
            return false;
          }
          dst += it->second;
        }
        if (!interp.exec(str, body.right)) return false;
        dst += str;

        //Increment the value
        map<wchar_t,wstring>::iterator it = interp.registers.find(reg);
        if (it == interp.registers.end()) {
          wcerr << L"for-integer loop register " << reg
                << L" was unset during execution." << endl;
          return false;
        }

        if (!parseInteger(curr, it->second)) {
          wcerr << L"for-integer loop register " << reg
                << L" was set to invalid integer " << it->second
                << " during execution." << endl;
          return false;
        }

        curr += sinc;
        it->second = intToStr(curr);
      }

      return true;
    }
  };

  template<bool EmitCounterImplicitly>
  class ForIntegerParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const wstring& text, unsigned& offset) {
      auto_ptr<Command> limit, init, inc;
      wchar_t reg = L'i';
      AutoSection body;
      ArgumentParser a(interp, text, offset, out);

      if (!a[a.h(),
             a.s(body) |
             (a.a(limit), a.s(body) |
              (a.h(reg), a.s(body) |
               (a.a(init), a.s(body) |
                (a.a(inc), a.s(body)))))])
        return ParseError;

      out = new ForInteger(out, EmitCounterImplicitly,
                           reg, init, limit, inc, body);
      body.clear();
      return ContinueParsing;
    }
  };

  static GlobalBinding<ForIntegerParser<false> >
  _forIntegerParser(L"for-integer");
  static GlobalBinding<ForIntegerParser<true > >
  _forIntPrintParser(L"for-int-print");

  class ForEach: public Command {
    wstring registers;
    Tokeniser tokeniser;
    AutoSection list, body;
    bool emitItemImplicitly;

  public:
    ForEach(Command* left,
            const wstring& registers_,
            const Tokeniser& tokeniser_,
            const Section& list_,
            const Section& body_,
            bool emitItemImplicitly_)
    : Command(left),
      registers(registers_),
      tokeniser(tokeniser_),
      list(list_),
      body(body_),
      emitItemImplicitly(emitItemImplicitly_)
    { }

    virtual bool exec(wstring& dst, Interpreter& interp) {
      wstring text, item;
      if (!list.exec(text, interp)) return false;
      tokeniser.reset(text);

      dst.clear();

      while (tokeniser.hasMore()) {
        for (unsigned i = 0; i < registers.size() && tokeniser.next(item); ++i)
          interp.registers[registers[i]] = item;

        if (tokeniser.error()) break;

        if (!interp.exec(text, body.left)) return false;
        dst += text;
        if (emitItemImplicitly)
          dst += item;
        if (!interp.exec(text, body.right)) return false;
        dst += text;
      }

      //Successful iff the tokeniser didn't fail.
      return !tokeniser.error();
    }
  };

  template<bool EmitItemImplicitly>
  class ForEachParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp,
                              Command*& out,
                              const wstring& text,
                              unsigned& offset) {
      wstring
        registers(L"p"),
        preprocessor(L"default-tokeniser-pre"),
        tokeniser(L"default-tokeniser"),
        options(L"");
      unsigned preprocessorOffset(offset), tokeniserOffset(offset);
      bool prependPlus(false), prependMinus(false);
      AutoSection list, body;
      ArgumentParser a(interp, text, offset, out);
      Function preprocessorFun, tokeniserFun;

      if (!a[a.h(),
             -a.an(registers),
             -(a.x(L'%'), a.to(preprocessor, L'%') >> preprocessorOffset),
             -(a.x(L'#'), a.to(tokeniser, L'#') >> tokeniserOffset),
             -(a.x(prependPlus, L'+') | a.x(prependMinus, L'-'),
               a.ns(options)),
             ((a.x(L'?'), a.s(body), a.s(list)) |
              (           a.s(list), a.s(body)))])
        return ParseError;

      if (prependPlus)
        options = L'+' + options;
      else if (prependMinus)
        options = L'-' + options;

      if (!interp.commandsL.count(preprocessor)) {
        interp.error(wstring(L"No such command: ") + preprocessor,
                     text, preprocessorOffset);
        return ParseError;
      }
      if (!interp.commandsL.count(tokeniser)) {
        interp.error(wstring(L"No such command: ") + tokeniser,
                     text, tokeniserOffset);
        return ParseError;
      }

      if (!interp.commandsL[preprocessor]->function(preprocessorFun)) {
        interp.error(wstring(L"Not a function: ") + preprocessor,
                     text, preprocessorOffset);
        return ParseError;
      }
      if (!interp.commandsL[tokeniser]->function(tokeniserFun)) {
        interp.error(wstring(L"Not a function: ") + tokeniser,
                     text, tokeniserOffset);
        return ParseError;
      }

      if (!preprocessorFun.compatible(2,2)) {
        interp.error(wstring(L"Incompatible with (2 <- 2): ") + preprocessor,
                     text, preprocessorOffset);
        return ParseError;
      }
      if (!tokeniserFun.compatible(2,2)) {
        interp.error(wstring(L"Incompatible with (2 <- 2): ") + tokeniser,
                     text, tokeniserOffset);
        return ParseError;
      }

      //OK, the call is good
      out = new ForEach(out, registers,
                        Tokeniser(interp, preprocessorFun, tokeniserFun,
                                  L"", options),
                        list, body, EmitItemImplicitly);
      list.clear();
      body.clear();
      return ContinueParsing;
    }
  };

  static GlobalBinding<ForEachParser<false> > _forEach(L"for-each");
  static GlobalBinding<ForEachParser<true > > _forEachP(L"for-each-print");
}
