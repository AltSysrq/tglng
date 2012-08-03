#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include "interp.hxx"
#include "common.hxx"

using namespace std;
using namespace tglng;

int main(int argc, const char*const* argv) {
  wstring out;
  Interpreter interp;
  if (interp.exec(out, wcin, Interpreter::ParseModeLiteral)) {
    wcout << out;
    return 0;
  } else {
    wcerr << "Failed." << endl;
    return EXIT_EXEC_ERROR_IN_INPUT;
  }
}
