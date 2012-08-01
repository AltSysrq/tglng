#ifndef INTERP_HXX_
#define INTERP_HXX_

#include <string>
#include <map>
#include <iostream>

#include "parse_result.hxx"

namespace tglng {
  class CommandParser;
  class Command;

  /**
   * Encapsulates the data associated with a TglNG interpreter as well as its
   * higher-level behaviours.
   */
  class Interpreter {
    //Not defined
    Interpreter(const Interpreter&);
  public:
    /**
     * Maps wstrings to CommandParser*s used to interperet the commands. These
     * CommandParser*s are owned by the Interpreter, and are freed in its
     * destructor.
     */
    std::map<std::wstring,CommandParser*> commandsL;
    /**
     * Maps wchar_ts to CommandParser*s used for the short names of the
     * commands. The CommandParser*s are not owned by this field, but should
     * also exist in commandsL (or that of a parent Interpreter this one was
     * cloned from).
     */
    std::map<wchar_t,CommandParser*> commandsS;
    /**
     * Maps wchar_ts to register values. If an entry is not present, that
     * register does not exist.
     */
    std::map<wchar_t,std::wstring> registers;

    /**
     * The current escape character.
     */
    wchar_t escape;

    /**
     * Creates a new Interpreter with only the builtin commands defined, no
     * registers, and the only short name '#' bound to "long-command".
     */
    Interpreter();
    /**
     * Creates a temporary, subordinate "copy" of the given Interpreter.
     *
     * The CommandParser*s in commandsL simply refer to those contained in and
     * owned by the parent. This means that this Interpreter is dependent on
     * the continued existence of the parent and the commands it had when it
     * was created.
     */
    Interpreter(const Interpreter* parent);
    ~Interpreter();

    /**
     * Defines the possible modes of parsing.
     */
    enum ParseMode {
      ///Every character is treated as self-insert.
      ParseModeVerbatim,
      ///Characters other than escape are treated as self-insert, others are
      ///single-command.
      ParseModeLiteral,
      ///Each character is looked up in commandsS, other than meta, which is
      ///assumed to be self-insert.
      ParseModeCommand
    };

    /**
     * Parses one command from the input text.
     *
     * @param out The left-hand command, or NULL if this is the first
     * command. On return, this is either unmodified or points to the new
     * top-level command.
     * @param text The text to parse.
     * @param off The offset of the character in text to examine for
     * parsing. On return, points to the first character not parsed.
     * @param mode The ParseMode to use.
     * @return The ParseResult returned by the encountered command, or
     * ParseError if the command could not be found.
     */
    ParseResult parse(Command*& out, const std::wstring& text, unsigned& off,
                      ParseMode mode);
    /**
     * Parses commands in the input text until all text is consumed or a
     * command parser indcates that parsing should terminate.
     *
     * @param out Undefined on input. If parsing is successful, set to a
     * Command* which is the top-level of the command tree.
     * @param text The text to parse.
     * @param off The offset within text of the first character to parse. On
     * return, points to the first character not parsed.
     * @param mode The parsing mode to use. Note that ParseModeVerbatim will
     * always consume the entire string (starting at off).
     * @return The ParseResult returned by the last CommandParser to run, or
     * ParseError if an unknown short command was encountered, or
     * StopEndOfInput if the end of the string was reached.
     */
    ParseResult parseAll(Command*& out, const std::wstring& text, unsigned off,
                         ParseMode mode);

    /**
     * Executes the given command in this interpreter, storing the result in
     * out. This is equivalent to command->exec(out, interp);, but is provided
     * for consistency. Returns true if successful, false otherwise.
     */
    bool exec(std::wstring& out, Command*);
    /**
     * Parses and executes the given string in the given parse mode, storing
     * the result in out. Returns true if all was successful, false otherwise.
     */
    bool exec(std::wstring& out, const std::wstring& text, ParseMode);
    /**
     * Reads all text from the given wistream, then parses it in the given
     * ParseMode, then executes the result, storing the command tree's result
     * in out.
     *
     * Returns true if all was successful, false otherwise.
     */
    bool exec(std::wstring& out, std::wistream& in, ParseMode);

    /**
     * Registers the given CommandParser* to the globally-predifined map. This
     * will only affect new Interpreters. The CommandParser* will never be
     * freed.
     */
    static void bindGlobal(const std::wstring&, CommandParser*);
  };

  /**
   * Used to set up an automatic global binding.
   *
   * Usage: static GlobalBinding<MyCommandParserClass> smnam("symbolic-name");
   */
  template<typename ParserClass>
  class GlobalBinding {
  public:
    GlobalBinding(const std::wstring& name) {
      Interpreter::bindGlobal(name, new ParserClass());
    }
  };
}

#endif /* INTERP_HXX_ */
