#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include "../command.hxx"
#include "../interp.hxx"
#include "../argument.hxx"

using namespace std;

namespace tglng {
  /**
   * Simple command which just executes a Section.
   */
  class SectionCommand: public Command {
    AutoSection section;

  public:
    SectionCommand(Command* left, Section sec)
    : Command(left), section(sec) {}

    virtual bool exec(wstring& out, Interpreter& interp) {
      out = L"";
      wstring tmp;
      if (section.left) {
        if (!interp.exec(tmp, section.left)) return false;
        out += tmp;
      }
      if (section.right) {
        if (!interp.exec(tmp, section.right)) return false;
        out += tmp;
      }
      return true;
    }
  };

  /**
   * Parser which simply takes a section (starting from its own character) and
   * produces a command which executes it.
   */
  class SectionCommandParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp,
                              Command*& out,
                              const wstring& text,
                              unsigned& offset) {
      Section section;
      ArgumentParser a(interp, text, offset, out);
      if (!a[a.s(section)]) return ParseError;

      out = new SectionCommand(out, section);
      return ContinueParsing;
    }
  };

  static GlobalBinding<SectionCommandParser>
      _sectionCommandParser(L"section-command");

  template<ParseResult What>
  class CloseParenParser: public CommandParser {
  public:
    virtual ParseResult parse(Interpreter& interp,
                              Command*& out,
                              const wstring& text,
                              unsigned& offset) {
      //Advance beyond the command char
      ++offset;

      return What;
    }
  };

  static GlobalBinding<CloseParenParser<StopCloseParen> >
      _closeParenParser(L"close-paren");
  static GlobalBinding<CloseParenParser<StopCloseBracket> >
      _closeBracketParser(L"close-bracket");
  static GlobalBinding<CloseParenParser<StopCloseBrace> >
      _closeBraceParser(L"close-brace");
}
