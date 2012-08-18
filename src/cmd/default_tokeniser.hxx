#ifndef CMD_DEFAULT_TOKENISER_HXX_
#define CMD_DEFAULT_TOKENISER_HXX_

#include <string>

namespace tglng {
  /**
   * Default tokeniser preprocessor.
   *
   * Iff options.coalesceDelims is true, skip all delimiters in the input
   * string.
   */
  bool defaultTokeniserPreprocessor(std::wstring* out, const std::wstring* in,
                                    Interpreter& interp, unsigned);
  /**
   * Default tokeniser implementation.
   */
  bool defaultTokeniser(std::wstring* out, const std::wstring* in,
                        Interpreter& interp, unsigned);
}

#endif /* CMD_DEFAULT_TOKENISER_HXX_ */
