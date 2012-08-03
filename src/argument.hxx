#ifndef ARGUMENT_HXX_
#define ARGUMENT_HXX_

#include <string>

namespace tglng {
  class Command;
  class Interpreter;

  /**
   * Encapsulates the data and basic semantics for argument extraction.
   */
  class Argument {
  public:
    Interpreter& interp;
    const std::wstring& text;
    unsigned& offset;

  protected:
    Command*& left;

    Argument(Interpreter&, const std::wstring&, unsigned&, Command*&);

  public:
    /**
     * Tests whether the string matches.
     *
     * Default advances past all whitespace and then returns whether a
     * non-whitespace character was found.
     */
    bool match();
  };

#define CTOR(clazz) \
  clazz##Argument(Interpreter& i, const std::wstring& s, \
                  unsigned& u, Command*& c)              \
  : Argument(i,s,u,c) {}

  /**
   * Extracts a single non-whitespace character as an argument.
   */
  class CharArgument: public Argument {
  public:
    CTOR(Char)
    typedef wchar_t get_t;
    bool get(get_t&);
  };

  /**
   * Extracts a numeric literal as an argument.
   */
  class NumericArgument: public Argument {
  public:
    CTOR(Numeric)
    typedef signed get_t;
    bool get(get_t&);
    bool match();
  };

  /**
   * Extracts and parses a single Command as an argument.
   */
  class CommandArgument: public Argument {
  public:
    CTOR(Command)
    typedef Command* get_t;
    bool get(get_t&);
  };

  /**
   * Defines the type of a section arguement.
   */
  struct Section {
    Command* left, * right;
  };
  /**
   * Extracts a Section argument type from input.
   */
  class SectionArgument: public Argument {
  public:
    CTOR(Section)
    typedef Section get_t;
    bool get(get_t&);
    bool match();
  };

  /**
   * Extracts a string from point to the given sentinal. Does not match if the
   * sentinel is not found.
   */
  class SentinelStringArgument: public Argument {
    wchar_t sentinel;

  public:
    SentinelStringArgument(Interpreter&, const std::wstring&, unsigned&,
                           Command*&, wchar_t);
    typedef std::wstring get_t;
    bool get(get_t&);
    bool match();
  };

  /**
   * Extracts a non-empty string of alphanumeric characters starting at point
   * and continuing to (but not including) the first non-alphanumeric character
   * or end of input.
   */
  class AlnumStringArgument: public Argument {
  public:
    CTOR(AlnumString)
    typedef std::wstring get_t;
    bool get(get_t&);
    bool match();
  };

  /**
   * Extracts a non-empty string of characters starting at point and continuing
   * to (but not including) the first whitespace character, the first character
   * which is a valid section specifier, or end of input.
   */
  class NonSectionStringArgument: public Argument {
  public:
    CTOR(NonSectionString)
    typedef std::wstring get_t;
    bool get(get_t&);
    bool match();
  };

  /**
   * Extracts either a single parsed command, or a literal number which is
   * wrapped in a command and returned.
   */
  class ArithmeticArgument: public Argument {
  public:
    CTOR(Arithmetic)
    typedef Command* get_t;
    bool get(get_t&);
  };

  /**
   * Matches a single character. If matched, extracts true to a boolean.
   */
  class ExactCharacterArgument: public Argument {
    wchar_t expect;

  public:
    ExactCharacterArgument(Interpreter&, const std::wstring&, unsigned&,
                           Command*&, wchar_t);
    typedef bool get_t;
    bool get(get_t&);
    bool match();
  };
#undef CTOR

  /**
   * Wraps the Argument classes to store the destination reference within.
   */
  template<typename T>
  class ArgumentExtractor {
  protected:
    T it;
    typename T::get_t& dst;

  public:
    ArgumentExtractor(const T& t, typename T::get_t& d)
    : it(t), dst(d)
    { }

    bool match() { return it.match(); }
    bool get() { return it.get(dst); }

    Interpreter& interp() { return it.interp; }
    const std::wstring& text() { return it.text; }
    unsigned offset() { return it.offset; }
  };

  /**
   * Chains two ArgumentExtractor-compatible classes into one.
   */
  template<typename First, typename Rest>
  class ArgumentSequence {
    First first;
    Rest rest;

  public:
    ArgumentSequence(const First& f, const Rest& r)
    : first(f), rest(r) {}

    bool match() { return first.match(); }
    bool get() {
      if (!first.get()) return false;
      if (!rest.match()) {
        interp().error(L"Could not match next part of argument sequence.",
                       text(), offset());
        return false;
      }
      return rest.get();
    }

    Interpreter& interp() { return first.interp(); }
    const std::wstring& text() { return first.text(); }
    unsigned offset() { return first.offset(); }
  };

  /**
   * Tries to match with one ArgumentExtractor-compatible class, then falls
   * back on the other.
   */
  template<typename First, typename Rest>
  class ArgumentOptions {
    First first;
    Rest rest;

  public:
    ArgumentOptions(const First& f, const Rest& r)
    : first(f), rest(r) {}

    bool match() { return first.match() || rest.match(); }
    bool get() { return first.match()? first.get() : rest.get(); }

    Interpreter& interp() { return first.interp(); }
    const std::wstring& text() { return first.text(); }
    unsigned offset() { return first.offset(); }
  };

  /**
   * Wraps an ArgumentExtractor-compatible class to make it optional.
   */
  template<typename T>
  class OptionalArgument {
    T it;

  public:
    OptionalArgument(const T& t) : it(t) {}

    bool match() { return true; }
    bool get() { return !it.match() || it.get(); }

    Interpreter& interp() { return it.interp(); }
    const std::wstring& text() { return it.text(); }
    unsigned offset() { return it.offset(); }
  };

  /**
   * Wraps an ArgumentExtractor-compatible class to save the offset when
   * parsing started.
   */
  template<typename T>
  class SaveArgumentOffset {
    T it;
    unsigned& dst;

  public:
    SaveArgumentOffset(const T& t, unsigned& u) : it(t), dst(u) {}

    bool match() { return it.match(); }
    bool get() {
      dst = offset();
      return it.get();
    }

    Interpreter& interp() { return it.interp(); }
    const std::wstring& text() { return it.text(); }
    unsigned offset() { return it.offset(); }
  };

  /**
   * Adds operators to ArgumentExtractor which give a nice syntax sugar for
   * compound arguments:
   *
   *   arg1, arg2  -> ArgumentSequence<arg1,arg2>
   *   arg1 | arg2 -> ArgumentOptions<arg1,arg2>
   *   -arg1       -> OptionalArgument<arg1>
   *   arg >> off  -> SaveArgumentOffset<arg>(off)
   */
  template<typename Contained>
  class ArgumentSyntaxSugar {
  public:
    Contained it;

    typedef Contained contained_t;

    ArgumentSyntaxSugar(const Contained& t)
    : it(t) {}

    bool match() { return it.match(); }
    bool get() { return it.get(); }

    template<typename Next>
    ArgumentSyntaxSugar<ArgumentSequence<Contained,
                                         typename Next::contained_t> >
    operator,(const Next& next) const {
      return
        ArgumentSyntaxSugar<
          ArgumentSequence<
            Contained,
            typename Next::contained_t> >(
              ArgumentSequence<Contained,typename Next::contained_t>(it,
                                                                     next.it));
    }

    template<typename Other>
    ArgumentSyntaxSugar<ArgumentOptions<Contained,
                                        typename Other::contained_t> >
    operator|(const Other& other) const {
      return
        ArgumentSyntaxSugar<
          ArgumentOptions<
            Contained,
            typename Other::contained_t> >(
              ArgumentOptions<Contained,
                              typename Other::contained_t>(it, other.it));
    }

    ArgumentSyntaxSugar<OptionalArgument<Contained> >
    operator-() const {
      return ArgumentSyntaxSugar<OptionalArgument<Contained> >(
        OptionalArgument<Contained>(it));
    }

    ArgumentSyntaxSugar<SaveArgumentOffset<Contained> >
    operator>>(unsigned& dst) const {
      return ArgumentSyntaxSugar<SaveArgumentOffset<Contained> >(
        SaveArgumentOffset<Contained>(it, dst));
    }
  };

  /**
   * Simplifies the construction and testing of argument extractors.
   */
  class ArgumentParser {
    Interpreter& interp;
    const std::wstring& text;
    unsigned& offset;
    unsigned startingOffset;
    Command*& left;

  public:
    /**
     * Creates an ArgumentParser with the given arguments required for all
     * Arguments.
     */
    ArgumentParser(Interpreter&, const std::wstring&,
                   unsigned&, Command*& left);

    ///Returns a wrapped CharArgument which discards its character.
    ArgumentSyntaxSugar<ArgumentExtractor<CharArgument> > h();
    ///Returns a wrapped CharArgument storing the character into the given
    ///destination
    ArgumentSyntaxSugar<ArgumentExtractor<CharArgument> > h(wchar_t&);
    ///Returns a wrapped NumericArgument storing the integer into the given
    ///destination.
    ArgumentSyntaxSugar<ArgumentExtractor<NumericArgument> > n(signed&);
    ///Returns a wrapped ArithmeticArgument storing the Command* into the given
    ///destination.
    ArgumentSyntaxSugar<ArgumentExtractor<ArithmeticArgument> > a(Command*&);
    ///Returns a wrapped SectionArgument storing the Section into the given
    ///destination.
    ArgumentSyntaxSugar<ArgumentExtractor<SectionArgument> > s(Section&);
    ///Returns a wrapped SentinelStringArgument storing the string into the
    ///given destination and terminating on the given character.
    ArgumentSyntaxSugar<ArgumentExtractor<SentinelStringArgument> >
    to(std::wstring&, wchar_t);
    ///Returns a wrapped AlnumStringArgument storing the string into the given
    ///destination.
    ArgumentSyntaxSugar<ArgumentExtractor<AlnumStringArgument> >
    an(std::wstring&);
    ///Returns a wrapped NonSectionStringArgument storing the string into the
    ///given destination.
    ArgumentSyntaxSugar<ArgumentExtractor<NonSectionStringArgument> >
    ns(std::wstring&);
    ///Returns an ExactCharacterArgument matching the given character. The
    ///extracted boolean is dropped.
    ArgumentSyntaxSugar<ArgumentExtractor<ExactCharacterArgument> > x(wchar_t);
    ///Returns an ExactCharacterArgument storing the boolean into the given
    ///location and matching the given character.
    ArgumentSyntaxSugar<ArgumentExtractor<ExactCharacterArgument> >
    x(bool&, wchar_t);
    ///Returns a CommandArgument storing the command into the given destination.
    ArgumentSyntaxSugar<ArgumentExtractor<CommandArgument> > c(Command*&);

    /**
     * Tries to match the given compound argument agains the input. If matching
     * and parsing is successful, returns true. Otherwise, prints a diagnostic
     * and returns false.
     */
    template<typename T>
    bool operator[](const T& t_) {
      T t(t_);
      if (!t.match()) {
        diagnosticUnmatched();
        return false;
      }
      if (!t.get()) {
        diagnosticParseError();
        return false;
      }
      return true;
    }

  private:
    void diagnosticUnmatched() const;
    void diagnosticParseError() const;
  };
}

#endif /* ARGUMENT_HXX_ */
