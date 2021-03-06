#ifndef FUNCTION_HXX_
#define FUNCTION_HXX_

#include <string>
#include <vector>

#include "command.hxx"

namespace tglng {
  class Interpreter;

  /**
   * A Function is a special command (or command variant) which is dynamically
   * invokable. In particular, it must fulfil the following properties:
   * + It must be immutable, having no state other than the interpreter in
   *   which it runs.
   * + It must have a fixed arity.
   * + It can only take strings as arguments.
   *
   * The arity of a Function is expressed as
   *   numoutputs <- numinputs
   * Where numinputs and numoutputs are non-negative integers. Note in
   * particular that a Function may return multiple values.
   *
   * Symbolically, a certain type of Function (ie, based on the task it is
   * expected to perform) is written as
   *   (output0, output1) <- (input0, input1)
   */
  class Function {
  public:
    /**
     * The function pointer type which performs a Function's actual work.
     *
     * The callee need not worry about the size of the arrays; it may assume
     * they are the correct size.
     *
     * @param out An array of output strings, assumed to be equal in size to
     * this Function's outputArity. The contents of these strings is undefined
     * when the Function is executed. The callee may alter them arbitrarily,
     * leaving the desired result in each when returning successfully.
     * @param in An array of input strings, assumed to be equal in size to this
     * Function's inputArity.
     * @param interp The Interpreter in which the Function is being run.
     * @param parm An arbitrary integer for use by the function.
     * @return Whether the function succeeded.
     */
    typedef bool (*exec_t)(std::wstring* out, const std::wstring* in,
                           Interpreter&, unsigned parm);

    /**
     * The number of output arguments this Function takes.
     */
    unsigned outputArity;

    /**
     * The number of input arguments this Function takes.
     */
    unsigned inputArity;

    /**
     * Call to invoke the Function.
     *
     * @see Function::exec_t
     */
    exec_t exec;

    /**
     * Paramater to pass to exec.
     */
    unsigned parm;

    /**
     * Constructs an invalid Function.
     */
    Function()
    : outputArity(0), inputArity(0), exec(NULL)
    { }

    /**
     * Constructs a Function with the given parms.
     *
     * @param outputArity_ The number of output arguments this Function will
     * take.
     * @param inputArity_ The number of input arguments this Function will take.
     * @param exec_ The implementation of this Function.
     * @param parm_ The value for parm (maybe used by exec)
     */
    Function(unsigned outputArity_, unsigned inputArity_, exec_t exec_,
             unsigned parm_ = 0)
    : outputArity(outputArity_), inputArity(inputArity_),
      exec(exec_), parm(parm_)
    { }

    /**
     * Checks whether this Function matches the given arity, in output,input
     * order.
     *
     * @param outputArity The number of output arguments the caller expects.
     * @param inputArity The number of input arguments the caller expects.
     * @return Whether the actual arity matches the expected.
     */
    bool matches(unsigned outputArity, unsigned inputArity) const {
      return this->outputArity == outputArity && this->inputArity == inputArity;
    }

    /**
     * Checks whether this Function is "compatible" with the given expected
     * arity, in output,input order.
     *
     * A Function is compatible with an arity if the actual number of arguments
     * is less than or equal to the expected number of arguments for both
     * inputs. This means that a "compatible" Function may ignore some of its
     * inputs, and may not alter some of its outputs. This means that a caller
     * which uses this behaviour should set the outputs to something reasonable
     * before calling the function.
     */
    bool compatible(unsigned outputArity, unsigned inputArity) const {
      return this->outputArity <= outputArity && this->inputArity <= inputArity;
    }

    /**
     * Extracts a Function object from the command with the given long name
     * from the given interpreter.
     *
     * @param dst The destination to write to.
     * @param interp The Interpreter to use.
     * @param name The name of the command which should represent the function.
     * @param outputArity The output arity to expect.
     * @param inputArity The input arity to expect.
     * @param text The text being parsed which caused this lookup to be
     * performed (default empty string).
     * @param nameOffset The offset within text where the name begins (default
     * zero).
     * @param validate Member function to call to verify that the function is
     * usable; defaults to &Function::compatible, but matches may be used as
     * well.
     * @return Whether a compatible function was obtained.
     */
    static bool get(Function& dst,
                    const Interpreter& interp,
                    const std::wstring& name,
                    unsigned outputArity, unsigned inputArity,
                    const std::wstring& text = std::wstring(),
                    unsigned nameOffset = 0,
                    bool (Function::*validate)(unsigned,unsigned) const =
                    &tglng::Function::compatible);
  };

  /**
   * This class encapsulates the parsing of standard function syntax.
   */
  class FunctionParser: public CommandParser {
    Function fun;

  public:
    /**
     * Creates a FunctionParser for the given Function object.
     */
    FunctionParser(Function);

    virtual ParseResult parse(Interpreter&, Command*&,
                              const std::wstring&, unsigned&);
    virtual bool function(Function&) const;
  };

  /**
   * Allows specifying the parms for the Function argument of a FunctionParser
   * at compile time (for use with GlobalBinding).
   */
  template<unsigned OutputArity, unsigned InputArity, Function::exec_t Exec>
  class TFunctionParser: public FunctionParser {
  public:
    TFunctionParser()
    : FunctionParser(Function(OutputArity, InputArity, Exec)) {}
  };

  /**
   * Represents an arbitrary function invocation.
   * This class should be considered sealed except to
   * DynamicFunctionInvocation.
   */
  class FunctionInvocation: public Command {
    friend class DynamicFunctionInvocation;
    Function function;
    std::wstring outregs;
    //Can't use auto_ptrs in a vector
    std::vector<Command*> arguments;

  public:
    /**
     * Creates a FunctionInvocation with the given parms.
     *
     * @param fun The function to execute
     * @param outregs The registers to write secondary outputs to
     * @param args A list of Commands to execute for each input.
     */
    FunctionInvocation(Command*, Function fun,
                       const std::wstring& outregs,
                       const std::vector<Command*> args);

    virtual ~FunctionInvocation();
    virtual bool exec(std::wstring&, Interpreter&);
  };
}

#endif /* FUNCTION_HXX_ */
