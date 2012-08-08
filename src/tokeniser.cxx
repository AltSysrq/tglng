#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include "tokeniser.hxx"
#include "function.hxx"

using namespace std;

namespace tglng {
  static bool defaultTokeniserInit(wstring* out, const wstring* in,
                                   Interpreter&) {
    out[0] = in[0];
    out[1] = L"";
    return true;
  }

  const Function Tokeniser::defaultInit(2, 1, defaultTokeniserInit);

  Tokeniser::Tokeniser(Interpreter& interp_,
                       Function finit_,
                       Function fnext_,
                       const wstring& text,
                       const wstring& opts)
  : interp(interp_),
    finit(finit_), fnext(fnext_),
    options(opts), remainder(text),
    hasInit(false), errorFlag(false)
  { }

  Tokeniser::Tokeniser(Interpreter& interp_,
                       Function fnext_,
                       const wstring& text,
                       const wstring& opts)
  : interp(interp_),
    finit(defaultInit), fnext(fnext_),
    options(opts), remainder(text),
    hasInit(false), errorFlag(false)
  { }

  bool Tokeniser::next(wstring& dst) {
    if (!hasMore()) return false;

    //Get the next
    wstring in[2], out[2];
    in[0] = remainder;
    in[1] = options;
    if (!fnext.exec(out, in, interp)) {
      errorFlag = true;
      return false;
    }

    //OK
    remainder = out[1];
    dst = out[0];
    return true;
  }

  bool Tokeniser::hasMore() {
    //Early exit if something went wrong in the past.
    if (errorFlag) return false;

    //Run init if this hasn't happened yet
    if (!hasInit) {
      wstring in[2];
      in[0] = remainder;
      in[1] = options;
      if (!finit.exec(&remainder, in, interp)) {
        errorFlag = true;
        return false;
      }

      hasInit = true;
    }

    //Check whether there is anything more.
    return !remainder.empty();
  }

  bool Tokeniser::isExhausted() const {
    if (errorFlag) return true; //Can't do anything more
    if (!hasInit) return false; //Don't know
    //Normal conditions
    return remainder.empty();
  }
}
