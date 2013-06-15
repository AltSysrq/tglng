#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <clocale>
#include <locale>
#include <exception>
#include <stdexcept>

#include "interp.hxx"
#include "common.hxx"

using namespace std;
using namespace tglng;

int main(int argc, const char*const* argv) {
  wstring out;
  Interpreter interp;

  setlocale(LC_ALL, "");
  setlocale(LC_NUMERIC, "C");
  try {
    locale::global(locale(""));
  } catch (runtime_error& re) {
    /*
      When GNU libstdc++ is used on top of a non-GNU libc, no locales other
      than "C" are supported. On some versions of libstdc++, trying to set the
      default locale throws a runtime_error().

      See:
      http://gcc.gnu.org/ml/libstdc++/2003-02/msg00345.html

      Unfortunately, no --use-my-systems-libc-dammit option seems to exist yet,
      especially not one usable at compilation time.

      If we catch the runtime_error, just ignore it and carry on --- there's
      nothing else we can do.
    */
  }

  //Try to read from standard configuration file.
  //TODO: Change this to something more sensible
  {
    wifstream in("rc.default");
    if (!interp.exec(out, in, Interpreter::ParseModeCommand)) {
      wcerr << L"Error reading user library." << endl;
      return EXIT_EXEC_ERROR_IN_USER_LIBRARY;
    }
  }

  if (interp.exec(out, wcin, Interpreter::ParseModeLiteral)) {
    wcout << out;
    Interpreter::freeGlobalBindings();
    return 0;
  } else {
    wcerr << L"Failed." << endl;
    return EXIT_EXEC_ERROR_IN_INPUT;
  }
}
