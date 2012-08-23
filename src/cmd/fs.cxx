#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <locale>
#include <clocale>
#include <string>
#include <cstring>

/* Use the POSIX glob(3) function if available. On a native Windows port,
 * you'll need to make your own glob(3) implementation and define HAVE_GLOB, or
 * entirely reimplement ls.
 */
#ifdef HAVE_GLOB_H
#include <glob.h>
#endif

#include "../interp.hxx"
#include "../command.hxx"
#include "../function.hxx"
#include "list.hxx"
#include "../common.hxx"

using namespace std;

namespace tglng {
#ifdef HAVE_GLOB
//Define glob parms which might not exist
#ifndef GLOB_BRACE
#  define GLOB_BRACE 0
#endif
#ifndef GLOB_TILDE
#  define GLOB_TILDE 0
#endif
  bool fs_ls(wstring* out, const wstring* in,
             Interpreter& interp, unsigned) {
    glob_t results;

    out->clear();

    locale theLocale("");

    //Transcode in[0] to a narrow string
    typedef codecvt<wchar_t, char, mbstate_t> converter_t;
    const converter_t& converter =
      use_facet<converter_t>(theLocale);
    vector<char> pattern(in->size() * converter.max_length() + 1);
    mbstate_t mbstate;
    const wchar_t* from_next;
    char* end;
    converter_t::result conversionResult =
      converter.out(mbstate, in->data(), in->data() + in->size(),
                    from_next, &pattern[0], &pattern[0] + pattern.size() - 1,
                    end);
    if (conversionResult != converter_t::ok) {
      wcerr << L"Could not convert glob pattern to narrow string: "
            << *in << endl;
      return false;
    }

    //Ensure it is NUL-terminated
    *end = 0;

    //Perform the glob
    //If it fails, just return an empty list (*out is already empty);
    //otherwise, convert to a list.
    if (!glob(&pattern[0], GLOB_BRACE|GLOB_TILDE, NULL, &results)) {
      for (unsigned i = 0; i < results.gl_pathc; ++i) {
        //Convert the filename back to a wide string
        vector<wchar_t> str(strlen(results.gl_pathv[i]));
        const char* nfrom_next;
        wchar_t* wend;
        mbstate = mbstate_t();
        conversionResult =
          converter.in(mbstate, results.gl_pathv[i],
                       results.gl_pathv[i] + strlen(results.gl_pathv[i]),
                       nfrom_next, &str[0], &str[0] + str.size(), wend);
        if (conversionResult != converter_t::ok) {
          //Print a warning about this one
          cerr << "WARN: Could not convert narrow filename to wide string: "
               << results.gl_pathv[i] << endl;
          continue;
        }

        //OK, add to the list
        list::lappend(*out, wstring(&str[0], wend));
      }
    }

    return true;
  }

  static GlobalBinding<TFunctionParser<1,1,fs_ls> > _ls(L"ls");
#endif /* HAVE_GLOB */
}
