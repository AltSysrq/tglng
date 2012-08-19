#ifndef CMD_LIST_HXX_
#define CMD_LIST_HXX_

#include <string>

namespace tglng {
  class Interpreter;
  namespace list {
    /**
     * Escapes the given item so that it can be string-appended (or prepended)
     * to a list with a space as a separator.
     *
     * (escaped-string <- string-to-escape)
     */
    bool escape(std::wstring*, const std::wstring*, Interpreter&, unsigned);

    /**
     * Appends the given (unescaped) item to the given list.
     *
     * (list <- list item)
     */
    bool append(std::wstring*, const std::wstring*, Interpreter&, unsigned);

    /**
     * Returns the first item in the list, as well as the remainder.
     *
     * Fails if the list is empty. On failure, prints an error message if the
     * parm is zero.
     *
     * (car cdr <- list)
     */
    bool car(std::wstring*, const std::wstring*, Interpreter&, unsigned);

    /**
     * Splits the given list into the first element and the tail. Returns
     * whether the list was non-empty.
     *
     * list may be the same object as car and/or cdr.
     */
    bool lcar(std::wstring& car, std::wstring& cdr,
              const std::wstring& list, Interpreter&);

    /**
     * Returns the length of the given list.
     */
    unsigned llength(const std::wstring&, Interpreter& interp);

    /**
     * Returns the length of the given list.
     *
     * (length <- list)
     */
    bool length(std::wstring*, const std::wstring*, Interpreter&, unsigned);

    /**
     * Returns the item at the given index within the given list.
     *
     * (item <- list index)
     */
    bool ix(std::wstring*, const std::wstring*, Interpreter&, unsigned);

    /**
     * Applies the given function to each element in the list, returning a list
     * whose items are the outputs of that function.
     *
     * (list <- (output <- input) list)
     */
    bool map(std::wstring*, const std::wstring*, Interpreter&, unsigned);

    /**
     * Applies the given function to each item in the list with an
     * accumulator.
     *
     * (list <- (reduction <- accum item) list [init=""])
     */
    bool fold(std::wstring*, const std::wstring*, Interpreter&, unsigned);

    /**
     * Removes items in the given list for which the given function returns
     * false.
     *
     * (list <- (accpt? <- input) list)
     */
    bool filter(std::wstring*, const std::wstring*, Interpreter&, unsigned);

    /**
     * Zips the given lists together so that their elements are interleaved.
     * For example, {{a b c} {d e f} {g h i}} results in
     * {a d g b e h c f i}
     *
     * (list <- list-of-lists)
     */
    bool zip(std::wstring*, const std::wstring*, Interpreter&, unsigned);

    /**
     * Flattens the given list by treating each element as a list and
     * concatenating them all.
     * {{a b c} {d e f}} ==> {a b c d e f}
     *
     * (list <- list-of-lists)
     */
    bool flatten(std::wstring*, const std::wstring*, Interpreter&, unsigned);

    /**
     * Performs the reverse operation of zip by deinterleaving the items into
     * the specified number of lists (default, 2).
     * Ex: {a b c d e f g h i} 3 ==> {{a d g} {b e h} {c f i}}
     *
     * (list-of-lists <- list [stride=2])
     */
    bool unzip(std::wstring*, const std::wstring*, Interpreter&, unsigned);
  }
}

#endif /* CMD_LIST_HXX_ */
