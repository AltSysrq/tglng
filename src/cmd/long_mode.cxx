#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <cctype>

#include "../interp.hxx"
#include "../command.hxx"
#include "long_mode.hxx"
#include "../common.hxx"

using namespace std;

namespace tglng {
  bool LongModeCmdParser::isname(wchar_t ch) {
    return iswalnum(ch) || ch == L'-' || ch == L'_';
  }

  ParseResult LongModeCmdParser::parse(Interpreter& interp,
                                       Command*& out,
                                       const wstring& text,
                                       unsigned& offset) {
    unsigned origOffset(offset);
    wstring name;

    if (!isname(text[offset])) {
      interp.error(L"long-mode-cmd: Invalid invocation.", text, offset);
      return ParseError;
    }

    while (offset < text.size() && isname(text[offset]))
      ++offset;

    //Offset moved one past end
    --offset;

    name.assign(text, origOffset, offset-origOffset+1);

    map<wstring,CommandParser*>::const_iterator it =
      interp.commandsL.find(name);

    CommandParser* parser;

    if (it == interp.commandsL.end()) {
      //If it's a single character, try a short name
      if (name.size() == 1 && interp.commandsS.count(name[0])) {
        parser = interp.commandsS[name[0]];
      } else {
        interp.error(wstring(L"No such command: ") + name, text, origOffset);
        return ParseError;
      }
    } else {
      parser = it->second;
    }

    return parser->parse(interp, out, text, offset);
  }

  static GlobalBinding<LongModeCmdParser> _longModeCmdParser(L"long-mode-cmd");

  template<bool LongMode>
  class LongModeParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp,
                              Command*& out,
                              const wstring& text,
                              unsigned& offset) {
      ++offset; //Past our command character

      bool wasLong = interp.longMode;
      interp.longMode = LongMode;

      //Parse all commands
      ParseResult result = interp.parseAll(out, text, offset,
                                           Interpreter::ParseModeCommand);

      //Restore old long mode
      interp.longMode = wasLong;

      return result;
    }
  };

  static GlobalBinding<LongModeParser<true > > _longMode(L"long-mode");
  static GlobalBinding<LongModeParser<false> > _shortMode(L"short-mode");
}
