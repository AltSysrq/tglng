#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <memory>
#include <vector>
#include <iostream>

#include "function.hxx"
#include "command.hxx"
#include "argument.hxx"
#include "interp.hxx"
#include "common.hxx"

using namespace std;

namespace tglng {
  bool Function::get(Function& dst,
                     const Interpreter& interp,
                     const wstring& name,
                     unsigned outputArity,
                     unsigned inputArity,
                     const wstring& text,
                     unsigned nameOffset,
                     bool (Function::*validate)(unsigned, unsigned) const) {
    map<wstring,CommandParser*>::const_iterator it =
      interp.commandsL.find(name);
    if (it == interp.commandsL.end()) {
      if (text.empty())
        wcerr << L"tglng: error: In dynamic function lookup: "
              << L"No such command: " << name << endl;
      else
        interp.error(wstring(L"No such command: ") + name,
                     text, nameOffset);
      return false;
    }

    CommandParser* parser = it->second;
    if (!parser->function(dst)) {
      if (text.empty())
        wcerr << L"tglng: error: In dynamic function lookup: "
              << L"Not a function: " << name << endl;
      else
        interp.error(wstring(L"Not a function: ") + name,
                     text, nameOffset);
      return false;
    }

    if (!(dst.*validate)(outputArity,inputArity)) {
      if (text.empty())
        wcerr << L"tglng: error: In dynamic function lookup: "
              << L"Inappropriate function: " << name << endl;
      else
        interp.error(wstring(L"Inappropriate function: ") + name,
                     text, nameOffset);

      wcerr << L"tglng: note: "
            << L"Needed (" <<     outputArity << L" <- " <<     inputArity
            << L"), got (" << dst.outputArity << L" <- " << dst.inputArity
            << L")" << endl;
      return false;
    }

    return true;
  }

  FunctionInvocation::FunctionInvocation(Command* left,
                                         Function fun,
                                         const wstring& outregs_,
                                         const vector<Command*> args)
  : Command(left),
    function(fun),
    outregs(outregs_),
    arguments(args)
  { }

  FunctionInvocation::~FunctionInvocation() {
    for (unsigned i = 0; i < arguments.size(); ++i)
      delete arguments[i];
  }

  bool FunctionInvocation::exec(wstring& dst, Interpreter& interp) {
    vector<wstring> out(function.outputArity);
    vector<wstring> in (function.inputArity );
    wstring discard;
    //Evaluate the arguments
    for (unsigned i = 0; i < arguments.size(); ++i)
      if (!interp.exec(i < in.size()? in[i] : discard, arguments[i]))
        return false;

    //Call the function
    if (!function.exec(&out[0], &in[0], interp, function.parm))
      return false;

    //Set outregs
    for (unsigned i = 1; i < out.size() && i-1 < outregs.size(); ++i)
      interp.registers[outregs[i-1]] = out[i];

    //Result in primary output
    dst = out[0];
    return true;
  }

  FunctionParser::FunctionParser(Function fun_)
  : fun(fun_) {}

  bool FunctionParser::function(Function& dst) const {
    dst = fun;
    return true;
  }

  ParseResult FunctionParser::parse(Interpreter& interp,
                                    Command*& out,
                                    const wstring& text,
                                    unsigned& offset) {
    wstring outregs;
    vector<Command*> arguments;
    bool done = false;
    ArgumentParser a(interp, text, offset, out);
    //Read the base of the function call
    if (!a[a.h(), -(a.x(L'['), a.to(outregs, L']') /* Sentinel is consumed */),
           a.x(L'('), -a.x(done, L')')])
      goto error;

    //Read in arguments
    while (!done) {
      auto_ptr<Command> tmp;
      if (!a[a.a(tmp), a.x(L',') | a.x(done, L')')])
        goto error;

      arguments.push_back(tmp.release());
    }

    //OK
    out = new FunctionInvocation(out, fun,
                                 outregs, arguments);
    //The FunctionInvocation now has control of the contents of arguments, so
    //don't free them.
    return ContinueParsing;

    error:
    //Clean up the arguments
    for (unsigned i = 0; i < arguments.size(); ++i)
      delete arguments[i];
    return ParseError;
  }
}
