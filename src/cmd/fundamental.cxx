#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <map>

#include "fundamental.hxx"
#include "../command.hxx"
#include "../interp.hxx"
#include "../common.hxx"

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
}
