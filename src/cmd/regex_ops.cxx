#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <cctype>

#include "../command.hxx"
#include "../function.hxx"
#include "../argument.hxx"
#include "../interp.hxx"
#include "../regex.hxx"

using namespace std;

namespace tglng {
  bool rxSupport(wstring* out, const wstring*, Interpreter&, unsigned) {
    *out = regexLevelName;
    return true;
  }

  static GlobalBinding<TFunctionParser<1,0,rxSupport> >
  _rxSupport(L"rx-support");
}
