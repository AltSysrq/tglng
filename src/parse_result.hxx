#ifndef PARSE_RESULT_HXX_
#define PARSE_RESULT_HXX_

namespace tglng {
  /**
   * Indicates the status after a completed parsing step. Parsing only
   * continues if the value is false.
   */
  enum ParseResult {
    ///No special status, continue parsing.
    ContinueParsing = 0,
    ///Parsing should be terminated due to a closing parenthesis.
    StopCloseParen,
    ///Parsing should be terminated due to a closing bracket.
    StopCloseBracket,
    ///Parsing should be terminated due to a closing brace.
    StopCloseBrace,
    /**
     * Parsing is being stopped due to hitting the end of input.
     *
     * This is not to be returned by individual parsers, but rather passed to
     * callers of the parsing system to indicate why parsing stopped.
     */
    StopEndOfInput,
    ///Parsing failed for some reason
    ParseError
  };
}

#endif /* PARSE_RESULT_HXX_ */
