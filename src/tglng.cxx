#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(USE_GETOPT_LONG) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE 1
#endif

#include <iostream>
#include <fstream>
#include <clocale>
#include <locale>
#include <exception>
#include <stdexcept>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "interp.hxx"
#include "cmd/list.hxx"
#include "options.hxx"
#include "common.hxx"

using namespace std;
using namespace tglng;

static void parse_cmdline_args(unsigned, const char*const*);

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

  parse_cmdline_args(argc, argv);

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

static void print_usage(void);

static void parse_cmdline_args(unsigned argc, const char*const* argv) {
#ifdef USE_GETOPT_LONG
  static const struct option long_options[] = {
    { "help", 0, NULL, 'h' },
    { "file", 1, NULL, 'f' },
    { "no-chdir", 0, NULL, 'H' },
    { "config", 1, NULL, 'c' },
    { "no-system-config", 0, NULL, 'C' },
    { "script", 1, NULL, 'e' },
    { "register", 1, NULL, 'D' },
    { "dry-run", 0, NULL, 'd' },
    { "locate-parse-error", 0, NULL, 'l' },
    {0}
  };
#endif
  static const char short_options[] = "hf:Hc:Ce:D:dl";

  int cmdstat;
  wstring wstr;

  do {
#ifdef USE_GETOPT_LONG
    cmdstat = getopt_long(argc, const_cast<char**>(argv),
                          short_options, long_options, NULL);
#else
    cmdstat = getopt(argc, argv, short_options);
#endif
    switch (cmdstat) {
    case -1: break;
    case 'h':
    case '?':
      print_usage();
      if ('h' == cmdstat)
        exit(0);
      else
        exit(EXIT_INCORRECT_USAGE);
      break;

    case 'f':
      if (!ntbstowstr(operationalFile, optarg)) {
        wcerr << L"Unable to decode option for -f or --file" << endl;
        exit(EXIT_PLATFORM_ERROR);
      }
      break;

    case 'H':
      implicitChdir = false;
      break;

    case 'c':
      if (!ntbstowstr(wstr, optarg)) {
        wcerr << L"Unable to decode option for -c or --config" << endl;
        exit(EXIT_PLATFORM_ERROR);
      }

      tglng::list::lappend(userConfigs, wstr);
      break;

    case 'C':
      enableSystemConfig = false;
      break;

    case 'e':
      if (!ntbstowstr(wstr, optarg)) {
        wcerr << L"Unable to decode option for -e or --script" << endl;
        exit(EXIT_PLATFORM_ERROR);
      }

      scriptInputs.push_back(wstr);
      break;

    case 'D':
      if (!ntbstowstr(wstr, optarg)) {
        wcerr << L"Unable to decode option for -D or --define" << endl;
        exit(EXIT_PLATFORM_ERROR);
      }

      if (wstr.size() < 2) {
        wcerr << L"-D or --define must have an argument of the form X=..."
              << endl;
        exit(EXIT_INCORRECT_USAGE);
      }

      initialRegisters[wstr[0]] = wstr.substr(2);
      break;

    case 'd':
      dryRun = true;
      break;

    case 'l':
      locateParseError = true;
      break;

    default:
      wcerr << L"BUG: Unhandled cmdstat: " << cmdstat << endl;
      abort();
    }
  } while (-1 != cmdstat);

  if ((unsigned)optind != argc) {
    wcerr << L"Extraneous arguments after options" << endl;
    print_usage();
    exit(EXIT_INCORRECT_USAGE);
  }
}

static void print_usage(void) {
}
