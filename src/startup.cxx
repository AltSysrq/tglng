#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <string>
#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <list>

#include <unistd.h>

#include "startup.hxx"
#include "interp.hxx"
#include "cmd/list.hxx"
#include "options.hxx"
#include "common.hxx"

using namespace std;

namespace tglng {
  static string dirname(const string& filename) {
    size_t lastSlash = filename.find_last_of("/");
    if (0 == lastSlash || string::npos == lastSlash)
      return "";

    return filename.substr(0, lastSlash-1);
  }

  static void chdirToFilename() {
    string filenameDir = dirname(operationalFile);
    if (!filenameDir.empty() && implicitChdir) {
      if (!chdir(filenameDir.c_str())) {
        cerr << "Failed to chdir() to " << filenameDir
             << ": " << strerror(errno);
        exit(EXIT_PLATFORM_ERROR);
      }
    }
  }

  static string homeRel(const string& basename) {
    const char* homeEnv = getenv("HOME");
    if (!homeEnv) homeEnv = "";

    return string(homeEnv) + "/" + basename;
  }

  static void slurpSet(set<string>& dst, const string& filename) {
    ifstream in(filename.c_str());
    if (!in) return;

    string item;
    while (getline(in, item, '\n'))
      dst.insert(item);
  }

  static void spitSet(const string& filename, const set<string>& dst) {
    ofstream out(filename.c_str());
    if (!out) return;

    for (set<string>::const_iterator it = dst.begin();
         it != dst.end(); ++it)
      out << *it << endl;
  }

  static void readConfig(Interpreter& interp, const string& filename) {
    wifstream in(filename.c_str());
    wstring discard;

    if (!in) return;

    if (!interp.exec(discard, in, Interpreter::ParseModeCommand))
      exit(EXIT_PARSE_ERROR_IN_USER_LIBRARY);
  }

  static bool readAuxConfigs(Interpreter& interp,
                             set<string>& known, const set<string>& permitted,
                             string directory) {
    const char* homeEnv = getenv("home");
    const string root("/"), home(homeEnv? homeEnv : "/");
    bool newKnown = false;

    while (!directory.empty() && directory != root && directory != home) {
      string path = directory + "/.tglng";
      if (access(path.c_str(), R_OK)) {
        if (permitted.count(directory)) {
          readConfig(interp, path);
        } else if (!known.count(directory)) {
          /* Warn the user about this once, then add it to known so we don't
           * keep bothering them.
           */
          cerr << "Note: Aux config " << path << " exists, but is not marked "
               << "as permitted." << endl
               << "Add \"" << directory << "\" to ~/.tglng_permitted if you "
               << "trust this script." << endl;
          known.insert(directory);
          newKnown = true;
        }
      }

      directory = dirname(directory);
    }

    return newKnown;
  }

  static void readUserConfiguration(Interpreter& interp) {
    if (!userConfigs.empty())
      for (std::list<string>::const_iterator it = userConfigs.begin();
           it != userConfigs.end(); ++it)
        readConfig(interp, *it);
    else
      readConfig(interp, homeRel(".tglng"));
  }

  void startUp(Interpreter& interp) {
    set<string> knownDirs, permittedDirs;

    chdirToFilename();

    if (enableSystemConfig) {
      readConfig(interp, "/usr/local/etc/tglngrc");
      readConfig(interp, "/usr/etc/tglngrc");
      readConfig(interp, "/etc/tglngrc");
    }

    slurpSet(knownDirs, homeRel(".tglng_known"));
    slurpSet(permittedDirs, homeRel(".tglng_permitted"));

    const char* cwd = getcwd(NULL, ~0);
    if (!cwd) {
      cerr << "getcwd() failed: " << strerror(errno) << endl;
      exit(EXIT_PLATFORM_ERROR);
    }

    if (readAuxConfigs(interp, knownDirs, permittedDirs, string(cwd)))
      spitSet(homeRel(".tglng_known"), knownDirs);

    free(const_cast<char*>(cwd));

    readUserConfiguration(interp);
  }
}
