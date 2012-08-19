#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <memory>

#include "../interp.hxx"
#include "../command.hxx"
#include "../argument.hxx"
#include "../common.hxx"

using namespace std;

namespace tglng {
  //Stores a reference-counted, shared string value.
  struct VariableValue {
    wstring value;
    unsigned refcount;
  };

  //Automatically handles refcounting on VariableValue
  class Variable {
    VariableValue*const value;

  public:
    Variable()
    : value (new VariableValue)
    {
      value->refcount=1;
    }

    Variable(VariableValue* v)
    : value(v)
    {
      ++value->refcount;
    }

    Variable(const wstring& str)
    : value(new VariableValue)
    {
      value->value = str;
      value->refcount = 1;
    }

    Variable(const Variable& that)
    : value(that.value)
    {
      ++value->refcount;
    }

    ~Variable() {
      if (!--value->refcount)
        delete value;
    }

    wstring& get() {
      return value->value;
    }
  };

  class VariableSet: public Command {
  protected:
    Variable var;

  private:
    auto_ptr<Command> value;

  public:
    VariableSet(Command* left,
                const Variable& var_,
                auto_ptr<Command>& value_)
    : Command(left), var(var_), value(value_)
    { }

    virtual bool exec(wstring& dst, Interpreter& interp) {
      dst.clear();
      wstring val;
      if (interp.exec(val, value.get())) {
        var.get() = val;
        return true;
      } else return false;
    }
  };

  class VariableLet: public VariableSet {
    auto_ptr<Command> body;

  public:
    VariableLet(Command* left,
                const Variable& var,
                auto_ptr<Command>& value,
                auto_ptr<Command>& body_)
    : VariableSet(left, var, value), body(body_)
    { }

    virtual bool exec(wstring& dst, Interpreter& interp) {
      wstring old(var.get());
      if (!VariableSet::exec(dst, interp))
        return false;

      bool result = interp.exec(dst, body.get());
      var.get() = old;
      return result;
    }
  };

  class VariableGet: public Command {
    Variable var;

  public:
    VariableGet(Command* left, const Variable& var_)
    : Command(left), var(var_) {}

    virtual bool exec(wstring& dst, Interpreter& interp) {
      dst = var.get();
      return true;
    }
  };

  class VariableGetParser: public CommandParser {
  public:
    Variable var;

    VariableGetParser(const Variable& var_)
    : var(var_) {}

    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const wstring& text, unsigned& offset) {
      ++offset; //Past "command char"
      out = new VariableGet(out, var);
      return ContinueParsing;
    }
  };

  class VariableLetParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const wstring& text, unsigned& offset) {
      wstring name;
      auto_ptr<Command> value, body;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.to(name, L'#'), a.x(L'='), a.a(value)])
        return ParseError;

      Variable var;

      //Create a temporary parser for the variable; first, keep whatever had
      //that command before.
      CommandParser* oldParser = interp.commandsL[name];
      CommandParser* newParser = new VariableGetParser(var);
      newParser->isTemporary = true;
      interp.commandsL[name] = newParser;

      //Get the body of the let
      Command* rawBody = NULL;
      ParseResult result = interp.parseAll(rawBody, text, offset,
                                           Interpreter::ParseModeCommand);
      body.reset(rawBody);

      //Restore the old command
      if (oldParser)
        interp.commandsL[name] = oldParser;
      else
        interp.commandsL.erase(name);
      delete newParser;

      //Create the let command if all OK
      if (result != ParseError)
        out = new VariableLet(out, var, value, body);

      return result;
    }
  };

  static GlobalBinding<VariableLetParser> _let(L"let");

  class VariableSetParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const wstring& text, unsigned& offset) {
      wstring name;
      auto_ptr<Command> value;
      unsigned nameOffset;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.to(name, L'#') >> nameOffset, a.x(L'='), a.a(value)])
        return ParseError;

      map<wstring,CommandParser*>::const_iterator it =
        interp.commandsL.find(name);

      if (it == interp.commandsL.end()) {
        interp.error(wstring(L"No such command: ") + name,
                     text, nameOffset);
        return ParseError;
      }

      VariableGetParser* vgp = dynamic_cast<VariableGetParser*>(it->second);
      if (!vgp) {
        interp.error(wstring(L"Not a variable (in this scope): ") + name,
                     text, nameOffset);
        return ParseError;
      }

      out = new VariableSet(out, vgp->var, value);
      return ContinueParsing;
    }
  };

  static GlobalBinding<VariableSetParser> _set(L"set");
}
