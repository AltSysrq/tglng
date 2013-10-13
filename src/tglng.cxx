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
#include <cerrno>
#include <cstring>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "interp.hxx"
#include "cmd/list.hxx"
#include "command.hxx"
#include "options.hxx"
#include "common.hxx"

using namespace std;
using namespace tglng;

static void parseCmdlineArgs(unsigned, const char*const*);
static void executePrimaryInput(Interpreter&, wistream&);
static void executePrimaryInput(Interpreter&, const string&);

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

  parseCmdlineArgs(argc, argv);

  //Try to read from standard configuration file.
  //TODO: Change this to something more sensible
  {
    wifstream in("rc.default");
    if (!interp.exec(out, in, Interpreter::ParseModeCommand)) {
      wcerr << L"Error reading user library." << endl;
      return EXIT_EXEC_ERROR_IN_USER_LIBRARY;
    }
  }

  if (scriptInputs.empty())
    executePrimaryInput(interp, wcin);
  else
    for (std::list<string>::const_iterator it = scriptInputs.begin();
         it != scriptInputs.end(); ++it)
      executePrimaryInput(interp, *it);

  Interpreter::freeGlobalBindings();
  return 0;
}

static void executePrimaryInput(Interpreter& interp, const string& filename) {
  wifstream in(filename.c_str());
  if (!in) {
    cerr << "Could not open " << filename << ": " << strerror(errno) << endl;
    exit(EXIT_IO_ERROR);
  }

  executePrimaryInput(interp, in);
}

static void executePrimaryInput(Interpreter& interp, wistream& in) {
  wstring text;
  Command* root = NULL;
  unsigned offset = 0;

  //Read all text to EOF (either real or UNIX)
  getline(in, text, L'\4');

  if (in.fail() && !in.eof()) {
    cerr << "Error reading input stream: " << strerror(errno) << endl;
    exit(EXIT_IO_ERROR);
  }

  switch (interp.parseAll(root, text, offset, Interpreter::ParseModeLiteral)) {
  case ContinueParsing: abort(); //Shouldn't happen
  case StopEndOfInput:
    break; //OK

  case StopCloseParen:
  case StopCloseBracket:
  case StopCloseBrace:
    interp.error(L"Unexpected closing parentheses, bracket, or brace.",
                 text, offset-1 /* -1 for back to command char */);

    /* Fall through */

  case ParseError:
    if (root) delete root;
    exit(EXIT_PARSE_ERROR_IN_INPUT);
  }

  if (!dryRun) {
    wstring out;
    bool res = interp.exec(out, root);

    if (!res) exit(EXIT_EXEC_ERROR_IN_INPUT);

    wcout << out;
  }

  delete root;
}

static void printUsage(bool);

static void parseCmdlineArgs(unsigned argc, const char*const* argv) {
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
      printUsage('h' != cmdstat);
      exit('h' == cmdstat? 0 : EXIT_INCORRECT_USAGE);
      break;

    case 'f':
      operationalFile = optarg;
      break;

    case 'H':
      implicitChdir = false;
      break;

    case 'c':
      userConfigs.push_back(std::string(optarg));
      break;

    case 'C':
      enableSystemConfig = false;
      break;

    case 'e':
      scriptInputs.push_back(std::string(optarg));
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
    printUsage(true);
    exit(EXIT_INCORRECT_USAGE);
  }
}

static void printUsage(bool error) {
  (error? wcerr : wcout) <<
    "Usage: tglng [OPTIONS...]\n"
    "Possible options are listed below. Arguments mandatory for long options\n"
    "are mandatory for the corresponding short options as well.\n"
    "  -h, -?, --help\n"
    "    Show this help message and exit.\n"
    "  -f, --file=<filename>\n"
    "    Indicates that the text produced by TglNG is expected to be added to\n"
    "    a file named by <filename>. By default, TglNG will chdir() into the\n"
    "    directory containing <filename>, unless --no-chdir is specified. It\n"
    "    is also possible for user configuration to change based on the value\n"
    "    of this option.\n"
    "  -H, --no-chdir\n"
    "    Suppress implicit chdir() into directory containing filename\n"
    "    specified via --file.\n"
    "  -c, --config=<file>\n"
    "    Instead of reading user configuration from ~/.tglng, read it from\n"
    "    <file>. This option may be specified multiple times; all listed\n"
    "    files will be read for user configuration.\n"
    "  -C, --no-system-config\n"
    "    Suppress implicit reading of system-wide configuration. Note that\n"
    "    quite a bit of TglNG functionality, including the handling of several\n"
    "    command line arguments listed here, is implemented in the default\n"
    "    system configuration.\n"
    "  -e, --script=<file>\n"
    "    Read primary program from <file> instead of standard input.\n"
    "    Specifying this argument multiple times causes all listed files to\n"
    "    be read and executed.\n"
    "  -D, --define=<definition>        <definition> ::= X=str\n"
    "    Specifies that register <X> will initially have value <str>. This is\n"
    "    applied whenever the reset-registers command is executed.\n"
    "  -d, --dry-run\n"
    "    Do everything but execute the primary input. In the case of multiple\n"
    "    files specified by --script, each listed file is parsed but not\n"
    "    executed.\n"
    "  -l, --locate-parse-error\n"
    "    If a parse error occurs, print the zero-based character offset of\n"
    "    the primary input where the error was encountered to standard\n"
    "    output, in addition to writing information about the error to\n"
    "    standard output.\n"
    #ifndef USE_GETOPT_LONG
    "\n(Long options are not available on your system.)"
    #endif
                         << endl;
}
