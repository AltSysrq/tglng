#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <cctype>
#include <cstdlib>
#include <iostream>

#include "argument.hxx"
#include "interp.hxx"
#include "cmd/fundamental.hxx"
#include "common.hxx"

using namespace std;

namespace tglng {
  static const wstring sectionTypes(L"<>:|([{$");

  Argument::Argument(Interpreter& interp_, const wstring& text_,
                     unsigned& offset_, Command*& left_)
  : interp(interp_), text(text_), offset(offset_), left(left_)
  { }

  bool Argument::match() {
    while (offset < text.size() && iswspace(text[offset]))
      ++offset;
    return offset < text.size();
  }

  bool CharArgument::get(wchar_t& out) {
    out = text[offset++];
    return true;
  }

  bool NumericArgument::match() {
    if (!Argument::match()) return false;

    wchar_t curr = text[offset];
    return
      curr == L'0' ||
      curr == L'1' ||
      curr == L'2' ||
      curr == L'3' ||
      curr == L'4' ||
      curr == L'5' ||
      curr == L'6' ||
      curr == L'7' ||
      curr == L'8' ||
      curr == L'9' ||
      curr == L'+' ||
      curr == L'-';
  }

  bool NumericArgument::get(signed& out) {
    return parseInteger(out, text, offset, &offset);
  }

  bool CommandArgument::get(Command*& out) {
    out = NULL;
    if (interp.parse(out, text, offset, Interpreter::ParseModeCommand))
      return true;
    else {
      interp.error(L"Invalid integer.", text, offset);
      return false;
    }
  }

  bool SectionArgument::match() {
    if (!Argument::match()) return false;

    return wstring::npos != sectionTypes.find(text[offset]);
  }

  bool SectionArgument::get(Section& out) {
    out.left = out.right = NULL;

    switch (text[offset++]) {
    case L'<':
      out.left = left;
      left = NULL;
      return true;

    case L'>':
      return interp.parseAll(out.right, text, offset,
                              Interpreter::ParseModeLiteral) != ParseError;

    case L':':
      return interp.parse(out.right, text, offset,
                           Interpreter::ParseModeCommand) == ContinueParsing;

    case L'|':
      out.left = left;
      if (interp.parseAll(out.right, text, offset,
                           Interpreter::ParseModeLiteral) != ParseError) {
        //Success, may nullify left then return
        left = NULL;
        return true;
      } else {
        //Since we are failing, out is considered to be meaningless. Thus we
        //cannot nullify left, since it is still the caller's responsibility.
        return false;
      }

    case L'(':
      return interp.parseAll(out.right, text, offset,
                              Interpreter::ParseModeCommand) == StopCloseParen;

    case L'[':
      return interp.parseAll(out.right, text, offset,
                              Interpreter::ParseModeLiteral) ==StopCloseBracket;

    case L'{': {
      //ParseModeVerbatim would consume the entire rest of the string, so do
      //our own conversion.
      unsigned cnt;
      unsigned start = offset;
      for (cnt = 1; cnt && offset < text.size(); ++offset)
        if (text[offset] == L'{')
          ++cnt;
        else if (text[offset] == L'}')
          --cnt;
      //After the loop, offset is one past the closing brace

      if (cnt) return false; //Unbalanced brace

      out.right = new SelfInsertCommand(NULL, text.substr(start,offset-start));
      return true;
    }

    case L'$':
      while (offset < text.size() && iswspace(text[offset])) ++offset;

      if (offset >= text.size()) return false;

      //TODO: out.right = new RegisterRead(text[offset++]);
      cerr << "FATAL: Don't know how to do register section yet!" << endl;
      exit(EXIT_THE_SKY_IS_FALLING);
      return true;
    }

    wcerr << L"FATAL: Unandled section type: " << text[offset-1] << endl;
    exit(EXIT_THE_SKY_IS_FALLING);
  }

  SentinelStringArgument::SentinelStringArgument(Interpreter& interp,
                                                 const wstring& text,
                                                 unsigned& offset,
                                                 Command*& left,
                                                 wchar_t sent)
  : Argument(interp, text, offset, left),
    sentinel(sent)
  { }

  bool SentinelStringArgument::match() {
    if (!Argument::match()) return false;

    if (sentinel == text[offset])
      //Forbid empty string
      return false;

    for (unsigned i = offset+1; i < text.size(); ++i)
      if (text[i] == sentinel)
        return true;

    return false;
  }

  bool SentinelStringArgument::get(wstring& dst) {
    unsigned start = offset;
    do {
      ++offset;
    } while (text[offset] != sentinel);

    dst = text.substr(start, offset-start);
    //Past the terminating character
    ++offset;
    return true;
  }

  bool AlnumStringArgument::match() {
    if (!Argument::match()) return false;
    //Use the char version of isalnum, since this is only used by commands
    //which assign special meaning to each character.
    //ANSI C does not define what happens if the character is outside of
    //0..127 (and indeed the MS C library crashes since it uses an array), so
    //do this check ourselves.
    //(Cast to char for clarity.)
    wchar_t curr = text[offset];
    return curr >= 0 && curr <= 127 && isalnum((char)curr);
  }

  bool AlnumStringArgument::get(wstring& dst) {
    unsigned start = offset;
    do {
      ++offset;
    //See comments in ::match() regarding this test.
    } while (text[offset] >= 0 && text[offset] <= 127 &&
             isalnum((char)text[offset]));

    dst = text.substr(start, offset-start);
    return true;
  }

  bool NonSectionStringArgument::match() {
    if (!Argument::match()) return false;

    return wstring::npos == sectionTypes.find(text[offset]);
  }

  bool NonSectionStringArgument::get(wstring& dst) {
    unsigned start = offset;
    do {
      ++offset;
    } while (wstring::npos == sectionTypes.find(text[offset]));

    dst = text.substr(start, offset-start);
    return true;
  }

  bool ArithmeticArgument::get(Command*& dst) {
    wchar_t fst = text[offset];
    if (fst >= L'0' && fst <= L'9') {
      unsigned start = offset;
      signed discard;
      if (!parseInteger(discard, text, start, &offset)) {
        //Invalid integer
        interp.error(L"Invalid integer.", text, offset);
        return false;
      }
      //Valid integer, but keep the original string representation in case it
      //matters
      dst = new SelfInsertCommand(NULL, text.substr(start, offset-start));
      return true;
    } else {
      return interp.parse(dst, text, offset, Interpreter::ParseModeCommand);
    }
  }

  ExactCharacterArgument::ExactCharacterArgument(Interpreter& interp,
                                                 const wstring& text,
                                                 unsigned& offset,
                                                 Command*& left,
                                                 wchar_t expect_)
  : Argument(interp, text, offset, left),
    expect(expect_)
  { }

  bool ExactCharacterArgument::match() {
    return Argument::match() && text[offset] == expect;
  }

  bool ExactCharacterArgument::get(bool& dst) {
    dst = true;
    ++offset;
    return true;
  }

  ArgumentParser::ArgumentParser(Interpreter& interp_,
                                 const wstring& text_,
                                 unsigned& offset_,
                                 Command*& left_)
  : interp(interp_), text(text_),
    offset(offset_), startingOffset(offset),
    left(left_)
  { }

  ArgumentSyntaxSugar<ArgumentExtractor<CharArgument> >
  ArgumentParser::h() {
    static wchar_t ignore;
    return ArgumentSyntaxSugar<ArgumentExtractor<CharArgument> >(
      ArgumentExtractor<CharArgument>(
        CharArgument(interp, text, offset, left),
        ignore));
  }

  ArgumentSyntaxSugar<ArgumentExtractor<CharArgument> >
  ArgumentParser::h(wchar_t& dst) {
    return ArgumentSyntaxSugar<ArgumentExtractor<CharArgument> >(
      ArgumentExtractor<CharArgument>(
        CharArgument(interp, text, offset, left),
        dst));
  }

  ArgumentSyntaxSugar<ArgumentExtractor<NumericArgument> >
  ArgumentParser::n(signed& dst) {
    return ArgumentSyntaxSugar<ArgumentExtractor<NumericArgument> >(
      ArgumentExtractor<NumericArgument>(
        NumericArgument(interp, text, offset, left),
        dst));
  }

  ArgumentSyntaxSugar<ArgumentExtractor<ArithmeticArgument> >
  ArgumentParser::a(Command*& dst) {
    return ArgumentSyntaxSugar<ArgumentExtractor<ArithmeticArgument> >(
      ArgumentExtractor<ArithmeticArgument>(
        ArithmeticArgument(interp, text, offset, left),
        dst));
  }

  ArgumentSyntaxSugar<ArgumentExtractor<SectionArgument> >
  ArgumentParser::s(Section& dst) {
    return ArgumentSyntaxSugar<ArgumentExtractor<SectionArgument> >(
      ArgumentExtractor<SectionArgument>(
        SectionArgument(interp, text, offset, left),
        dst));
  }

  ArgumentSyntaxSugar<ArgumentExtractor<SentinelStringArgument> >
  ArgumentParser::to(wstring& dst, wchar_t sentinel) {
    return ArgumentSyntaxSugar<ArgumentExtractor<SentinelStringArgument> >(
      ArgumentExtractor<SentinelStringArgument>(
        SentinelStringArgument(interp, text, offset, left, sentinel),
        dst));
  }

  ArgumentSyntaxSugar<ArgumentExtractor<AlnumStringArgument> >
  ArgumentParser::an(wstring& dst) {
    return ArgumentSyntaxSugar<ArgumentExtractor<AlnumStringArgument> >(
      ArgumentExtractor<AlnumStringArgument>(
        AlnumStringArgument(interp, text, offset, left),
        dst));
  }

  ArgumentSyntaxSugar<ArgumentExtractor<NonSectionStringArgument> >
  ArgumentParser::ns(wstring& dst) {
    return ArgumentSyntaxSugar<ArgumentExtractor<NonSectionStringArgument> >(
      ArgumentExtractor<NonSectionStringArgument>(
        NonSectionStringArgument(interp, text, offset, left),
        dst));
  }

  ArgumentSyntaxSugar<ArgumentExtractor<ExactCharacterArgument> >
  ArgumentParser::x(bool& dst, wchar_t expect) {
    return ArgumentSyntaxSugar<ArgumentExtractor<ExactCharacterArgument> >(
      ArgumentExtractor<ExactCharacterArgument>(
        ExactCharacterArgument(interp, text, offset, left, expect),
        dst));
  }

  ArgumentSyntaxSugar<ArgumentExtractor<ExactCharacterArgument> >
  ArgumentParser::x(wchar_t expect) {
    static bool ignore;
    return x(ignore, expect);
  }

  ArgumentSyntaxSugar<ArgumentExtractor<CommandArgument> >
  ArgumentParser::c(Command*& dst) {
    return ArgumentSyntaxSugar<ArgumentExtractor<CommandArgument> >(
      ArgumentExtractor<CommandArgument>(
        CommandArgument(interp, text, offset, left),
        dst));
  }

  void ArgumentParser::diagnosticUnmatched() const {
    interp.error(L"Could not match initial argument.", text, offset);
  }

  void ArgumentParser::diagnosticParseError() const {
    interp.error(L"Error reading argument for command.", text, startingOffset);
  }
}
