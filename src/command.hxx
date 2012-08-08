#ifndef COMMAND_HXX_
#define COMMAND_HXX_

#include <string>

#include "parse_result.hxx"

namespace tglng {
  class Interpreter;
  class Command;
  class Function;

  /**
   * Defines a method for converting input text into a Command.
   */
  class CommandParser {
  public:
    virtual ~CommandParser() {}
    /**
     * Tries to parse the given text into a command understood by this
     * CommandParser.
     *
     * @param interp The Interpreter in which the code will run.
     * @param accum On input, the left-hand command. If successful, set to the
     * new Command resulting. The callee may modify it, but if it does so, it
     * must ensure that the old value will be deleted.
     * @param text The code being parsed.
     * @param offset The offset of the command character within text. The
     * callee may modify it arbitrarily, but should leave it at the next
     * command on success, or at the problem point on error.
     * @return The parsing status after this command.
     */
    virtual ParseResult parse(Interpreter& interp,
                              Command*& accum,
                              const std::wstring& text,
                              unsigned& offset) = 0;

    /**
     * Represents the Command type processed by this CommandParser as a
     * Function, if supported.
     *
     * Default returns false.
     *
     * @param dst The Function to which to write the representation.
     * @return Whether this CommandParser supports a Function representation.
     */
    virtual bool function(Function& dst) const { return false; }
  };

  /**
   * Defines the interface for executing command trees.
   */
  class Command {
  public:
    /**
     * The Command-tree to the left of this command, or NULL if there is
     * nothing.
     *
     * This value belongs to this Command, and will be deleted when it is.
     */
    Command*const left;

  protected:
    /**
     * Constructs the Command with the given left-hand tree.
     */
    Command(Command*);

  public:
    virtual ~Command();
    /**
     * Executes this command. Calling Interpreter::exec() is preferred to this
     * function, as it handles the left-hand code tree as well.
     *
     * @param interp The Interpreter in which the Command is running.
     * @param out The result string. Its contents are undefined on input. The
     * Command may modify its value arbitrarily; the output value only matters
     * if successful.
     * @return True if execution was successful, false otherwise.
     * @see Interpreter::exec(std::wstring&, Command*)
     */
    virtual bool exec(std::wstring& out, Interpreter& interp) = 0;
  };
}

#endif /* COMMAND_HXX_ */
