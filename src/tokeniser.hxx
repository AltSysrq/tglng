#ifndef TOKENISER_HXX_
#define TOKENISER_HXX_

#include <string>

#include "function.hxx"

namespace tglng {
  class Interpreter;

  /**
   * A Tokeniser utilises two Functions to transform an input string (and
   * options) to a series of tokens.
   *
   * The first Funtcion, init, has the form
   *   (str) <- (str options)
   * That is, it takes the caller-specified input string and transforms it to a
   * string usable by the second Function. options is a user-supplied string of
   * ASCII alphanumeric characters. In most cases, the default init is used,
   * which copies str to str.
   *
   * The second Function, next, has the form
   *   (token remainder) <- (remainder options)
   * On input, remainder is what was returned by the previous invocation of
   * next, or was returned by str from init. options is a user-supplied string
   * of ASCII alphanumeric characters. token is the next token. If the output
   * remainder is the empty string, the sequence is considered exhausted.
   */
  class Tokeniser {
    Interpreter& interp;
    Function finit, fnext;
    std::wstring options, remainder;
    bool hasInit, errorFlag;

  public:
    /**
     * The default init Function.
     *
     * The input string is output verbatim, and options is ignored.
     */
    static const Function defaultInit;

    /**
     * Constructs a Tokeniser using the two given Functions.
     *
     * init will not be run until the first call to Tokeniser::next().
     *
     * @param interp The Interpreter to use.
     * @param init The Function to use for init.
     * @param next The Function to use for next.
     * @param text The text to tokenise.
     * @param opts The options to pass to init and next.
     */
    Tokeniser(Interpreter& interp, Function init, Function next,
              const std::wstring& text, const std::wstring& opts);

    /**
     * Constructs a Tokeniser using defaultInit for init.
     *
     * @param interp The Interpreter to use.
     * @param next The Function to use for next.
     * @param text The text to tokenise.
     * @param opts The options to pass to init and next.
     */
    Tokeniser(Interpreter& interp, Function next,
              const std::wstring& text, const std::wstring& opts);

    /**
     * Extracts the next token in the sequence, if possible.
     *
     * init will be called if it has not been yet. If remainder is not the
     * empty string, next is called and its result is used as the token, and
     * true is returned. Otherwise, false is returned. Once false has been
     * returned, no more tokens can be extracted from this Tokeniser.
     *
     * If one of the Functions fails, this function returns false and sets its
     * error flag to true.
     *
     * @param dst If another token can be extracted, it is written here.
     * @return Whether a token was extracted.
     */
    bool next(std::wstring& dst);

    /**
     * Indicates whether another token can be removed.
     *
     * This will run init if it has not done so, thus modifying the object (and
     * possibly the Interpreter and querying the OS!).
     *
     * If one of the Functions fails, this function returns false and sets its
     * error flag to true.
     *
     * @return Whether another token can be extracted.
     */
    bool hasMore();

    /**
     * Indicates whether the entire token sequence has been consumed.
     *
     * This function can NOT be used to determine that there are more tokens;
     * rather, it purely checks whether it can say, for certain, that the token
     * supply has been exhausted. In particular, if init has not been run, it
     * simply returns false, since the condition is actually unknown.
     */
    bool isExhausted() const;

    /**
     * Returns true if an error occurred while processing a Function.
     */
    bool error() const { return errorFlag; }
  };
}

#endif /* TOKENISER_HXX_ */
