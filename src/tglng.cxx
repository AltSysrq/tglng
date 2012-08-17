#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <clocale>
#include <locale>

#include "interp.hxx"
#include "common.hxx"

using namespace std;
using namespace tglng;

int main(int argc, const char*const* argv) {
  wstring out;
  Interpreter interp;

  setlocale(LC_ALL, "");
  setlocale(LC_NUMERIC, "C");
  locale::global(locale(""));

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
