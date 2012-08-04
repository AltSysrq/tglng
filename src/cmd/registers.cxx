#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>
#include <memory>

#include "registers.hxx"
#include "../interp.hxx"
#include "../argument.hxx"
#include "../common.hxx"

using namespace std;

namespace tglng {
  ReadRegister::ReadRegister(Command* left, wchar_t r)
  : Command(left), reg(r)
  { }

  bool ReadRegister::exec(wstring& dst, Interpreter& interp) {
    map<wchar_t,wstring>::const_iterator it = interp.registers.find(reg);
    if (it == interp.registers.end()) {
      wcerr << L"tgl: error: Attempt to read from unset register: "
            << reg << endl;
      return false;
    }

    dst = it->second;
    return true;
  }

  class WriteRegister: public Command {
    wchar_t reg;
    auto_ptr<Command> sub;

  public:
    WriteRegister(Command* left, wchar_t r, auto_ptr<Command>& s)
    : Command(left), reg(r), sub(s)
    { }

    virtual bool exec(wstring& dst, Interpreter& interp) {
      wstring res;
      if (!interp.exec(res, sub.get())) return false;

      interp.registers[reg] = res;
      dst = L"";
      return true;
    }
  };

  class UnsetRegister: public Command {
    wchar_t reg;

  public:
    UnsetRegister(Command* left, wchar_t r)
    : Command(left), reg(r)
    { }

    virtual bool exec(wstring& dst, Interpreter& interp) {
      interp.registers.erase(reg);
      dst = L"";
      return true;
    }
  };

  template<typename C>
  class RegCmdParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const wstring& text, unsigned& offset) {
      wchar_t reg;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.h(reg)]) return ParseError;

      out = new C(out, reg);
      return ContinueParsing;
    }
  };

  class WriteRegisterParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp, Command*& out,
                              const wstring& text, unsigned& offset) {
      auto_ptr<Command> sub;
      wchar_t reg;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.h(), a.h(reg), a.c(sub)]) return ParseError;

      out = new WriteRegister(out, reg, sub);
      return ContinueParsing;
    }
  };

  static GlobalBinding<RegCmdParser<ReadRegister> >
      _readRegParser(L"read-reg");
  static GlobalBinding<RegCmdParser<UnsetRegister> >
      _unsetRegParser(L"unset-reg");
  static GlobalBinding<WriteRegisterParser> _writeRegParser(L"write-reg");
}
