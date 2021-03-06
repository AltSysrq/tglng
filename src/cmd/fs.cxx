#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cerrno>

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

    //Transcode in[0] to a narrow string
    vector<char> pattern;
    if (!wstrtontbs(pattern, in[0])) {
      wcerr << L"Could not convert glob pattern to narrow string: "
            << *in << endl;
      return false;
    }

    //Perform the glob
    //If it fails, just return an empty list (*out is already empty);
    //otherwise, convert to a list.
    if (!glob(&pattern[0], GLOB_BRACE|GLOB_TILDE, NULL, &results)) {
      for (unsigned i = 0; i < results.gl_pathc; ++i) {
        wstring name;
        //Convert the filename back to a wide string
        if (!ntbstowstr(name, results.gl_pathv[i])) {
          //Print a warning about this one
          cerr << "WARN: Could not convert narrow filename to wide string: "
               << results.gl_pathv[i] << endl;
          continue;
        }

        //OK, add to the list
        list::lappend(*out, name);
      }

      globfree(&results);
    }

    return true;
  }

  static GlobalBinding<TFunctionParser<1,1,fs_ls> > _ls(L"ls");
#endif /* HAVE_GLOB */

  /**
   * Converts the given wstring to a char*, stored within dst. The actual size
   * of dst is not defined; rather, the only guarantee is that &dst[0] is a
   * valid NTBS.
   *
   * If successful, returns true; otherwise, prints a diagnostic and returns
   * false.
   */
  static bool wstrtofn(vector<char>& dst, const wstring& src) {
    bool success = wstrtontbs(dst, src);
    if (!success)
      wcerr << L"Could not convert filename to narrow string: "
            << src << endl;
    return success;
  }

  static void blitstr(wstring& dst, const wstring& src) {
    dst = src;
  }
  static void blitstr(wstring& dst, const string& src) {
    dst.resize(src.size());
    for (unsigned i = 0; i < src.size(); ++i)
      dst[i] = (wchar_t)(unsigned char)src[i];
  }
  static void blitstr(string& dst, const wstring& src) {
    dst.resize(src.size());
    for (unsigned i = 0; i < src.size(); ++i)
      dst[i] = (char)src[i];
  }

  template<typename Ifstream, typename String, bool Text>
  bool fs_read(wstring* out, const wstring* in,
               Interpreter& interp, unsigned) {
    vector<char> fname;
    if (!wstrtofn(fname, in[0]))
      return false;
    Ifstream input(&fname[0], Text? ios::in : ios::in | ios::binary);
    if (!input) {
      out[0].clear();
      out[1] = L"0";
      return true;
    }

    String str;
    static String eof((size_t)1, 4);
    getline(input, str, eof[0]);
    blitstr(out[0], str);

    if (!input.fail())
      out[1] = L"1";
    else
      out[1] = L"0";

    return true;
  }

  static GlobalBinding<TFunctionParser<2,1,fs_read<wifstream, wstring, true> > >
  _read(L"read");
  static GlobalBinding<TFunctionParser<
                         2,1,fs_read<ifstream, string, false> > >
  _readBinary(L"read-binary");

  template<typename Ofstream, typename String, bool Text, bool Append>
  bool fs_write(wstring* out, const wstring* in,
                Interpreter& interp, unsigned) {
    vector<char> fname;
    String content;
    if (!wstrtofn(fname, in[0]))
      return false;

    blitstr(content, in[1]);

    ios_base::openmode mode = ios::out;
    if (!Text)
      mode |= ios::binary;
    if (Append)
      mode |= ios::app;
    else
      mode |= ios::trunc;

    Ofstream output(&fname[0], mode);
    if (!output) {
      out[0] = L"0";
      return true;
    }

    output << content;

    out[0] = output? L"1" : L"0";
    return true;
  }

  static GlobalBinding<TFunctionParser<1,2,fs_write<wofstream,
                                                    wstring,
                                                    true,
                                                    false> > >
  _write(L"write");
  static GlobalBinding<TFunctionParser<1,2,fs_write<wofstream,
                                                    wstring,
                                                    true,
                                                    true> > >
  _append(L"append");
  static GlobalBinding<TFunctionParser<1,2,fs_write<ofstream,
                                                    string,
                                                    false,
                                                    false> > >
  _writeBinary(L"write-binary");
  static GlobalBinding<TFunctionParser<1,2,fs_write<ofstream,
                                                    string,
                                                    false,
                                                    true> > >
  _appendBinary(L"append-binary");
}
