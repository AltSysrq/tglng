#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "../command.hxx"
#include "../function.hxx"
#include "../interp.hxx"
#include "../common.hxx"

using namespace std;

namespace tglng {
  bool cmdGetenv(wstring* out, const wstring* in,
                 Interpreter& interp, unsigned) {
    vector<char> name;
    if (!wstrtontbs(name, *in)) {
      wcerr << L"Could not convert to narrow string for getenv: "
            << *in << endl;
      return false;
    }
    const char* value = getenv(&name[0]);
    if (value) {
      if (!ntbstowstr(out[0], value)) {
        wcerr << L"Could not widen string in getenv: "
              << value << endl;
        return false;
      }
      out[1] = L"1";
    } else {
      out[0].clear();
      out[1] = L"0";
    }
    return true;
  }

  bool cmdSetenv(wstring* out, const wstring* in,
                 Interpreter& interp, unsigned) {
    vector<char> name, value;
    if (!wstrtontbs(name, in[0]) ||
        !wstrtontbs(value, in[1])) {
      wcerr << L"Could not convert to narrow string for setenv: "
            << in[0] << endl << L"or: " << in[1] << endl;
      return false;
    }

    if (setenv(&name[0], &name[1], true)) {
      wcerr << L"Could not set " << in[0] << L" to " << in[1]
            << L": " << strerror(errno) << endl;
      return false;
    }

    out->clear();
    return true;
  }

  static GlobalBinding<TFunctionParser<2,1,cmdGetenv> >
  _getenv(L"getenv");
  static GlobalBinding<TFunctionParser<1,2,cmdSetenv> >
  _setenv(L"setenv");

  //The following function copied from Tgl, file src/builtins/external.c,
  //adapted for C++
  /* Invokes the specified command and arguments, all verbatim.
   *
   * If input is non-NULL, it's contents are piped into the child process's
   * standard input. Otherwise, the child process receives no input.
   *
   * The process's standard output is captured and accumulated into a
   * string. Standard error is inherited from Tgl.
   *
   * The standard output of the process is written to output, and the return
   * status to returnStatus. Returns whether execution was successful
   * (indipendent of return status).
   */
  static bool invoke_external(wstring& woutput, int& returnStatus,
                              char*const* argv, const wstring& winput) {
    FILE* output_file = NULL, * input_file = NULL;
    int output_fd, input_fd;
    long output_length, input_size;
    unsigned output_off;
    pid_t child;
    int child_status;
    vector<char> input, output;
    const char* input_end;
    if (!wstrtontbs(input, winput, &input_end)) {
      wcerr << L"tglng: error: Narrowing command input: "
            << strerror(errno) << endl;
      goto error;
    }
    input_size = input_end - &input[0];

    /* I believe the most portable way to do this is to use tmpfile() to open
     * two temporary files, one for input and one for output. Write the input
     * to that file, then execute the child, then read its output back in once
     * it dies.
     *
     * Of course, tmpfile() is broken on Windows, but then Windows doesn't have
     * fork() either, so regardless this function must be rewritten for that
     * platform.
     */
    input_file = tmpfile();
    if (!input_file) {
      fprintf(stderr, "tglng: error: creating temp input file: %s\n",
              strerror(errno));
      goto error;
    }

    output_file = tmpfile();
    if (!output_file) {
      fprintf(stderr, "tglng: error: creating temp output file: %s\n",
              strerror(errno));
      goto error;
    }

    if ((input_fd  = fileno(input_file )) == -1 ||
        (output_fd = fileno(output_file)) == -1) {
      fprintf(stderr, "tglng: unexpected error: %s\n", strerror(errno));
      goto error;
    }

    /* File descriptors set up; now write input */
    if (input_size) {
      if (!fwrite(&input[0], 1, input_size, input_file)) {
        fprintf(stderr, "tglng: error: writing command input: %s\n",
                strerror(errno));
        goto error;
      }
      rewind(input_file);
    }

    /* Execute the child */
    child = fork();
    if (child == -1) {
      fprintf(stderr, "tglng: error: fork: %s\n", strerror(errno));
      goto error;
    }

    if (!child) {
      /* Child process */
      if (-1 == dup2(input_fd, STDIN_FILENO) ||
          -1 == dup2(output_fd, STDOUT_FILENO)) {
        fprintf(stderr, "tglng: error: dup2: %s\n", strerror(errno));
        exit(-1);
      }
      /* Close original FDs */
      close(input_fd);
      close(output_fd);

      /* Switch to new process */
      execvp(argv[0], argv);
      /* If we get here, execvp() failed */
      fprintf(stderr, "tglng: error: execvp: %s\n", strerror(errno));
      exit(EXIT_PLATFORM_ERROR);
    }

    /* Parent process. Wait for child to die. */
    if (-1 == waitpid(child, &child_status, 0)) {
      fprintf(stderr, "tglng: error: waitpid: %s\n", strerror(errno));
      goto error;
    }

    /* Did the child exit successfully? */
    if (!WIFEXITED(child_status)) {
      fprintf(stderr, "tglng: error: child process %s terminated abnormally\n",
              argv[0]);
      goto error;
    }
    returnStatus = WEXITSTATUS(child_status);

    /* OK, get output */
    output_length = lseek(output_fd, 0, SEEK_END);
    if (output_length == -1 || -1 == lseek(output_fd, 0, SEEK_SET)) {
      fprintf(stderr, "tglng: error: lseek: %s\n", strerror(errno));
      goto error;
    }
    output.resize(output_length);
    for (output_off = 0; output_off < output.size() && output_length >= 1;
         output_off += output_length)
      output_length = read(output_fd, &output[0] + output_off,
                           output.size() - output_off);

    if (output_length == -1) {
      fprintf(stderr, "tglng: error: read: %s\n", strerror(errno));
      goto error;
    }
    if (output_length == 0 && output_off < output.size()) {
      fprintf(stderr, "tglng: error: EOF in encountered earlier than expected\n");
      fprintf(stderr, "tglng: This is likely a bug.\n");
      fprintf(stderr, "tglng: %s:%d\n", __FILE__, __LINE__);
      goto error;
    }

    //Decode output
    woutput.clear();
    if (output.size()) {
      if (!ntbstowstr(woutput, &output[0], &output[0] + output.size())) {
        fprintf(stderr, "tglng: error: Decoding output of command: %s\n",
                strerror(errno));
        goto error;
      }
    }

    /* Done */
    fclose(input_file);
    fclose(output_file);
    return true;

    error:
    if (input_file) fclose(input_file);
    if (output_file) fclose(output_file);
    return false;
  }

  bool cmdExec(wstring* out, const wstring* in,
               Interpreter& interp, unsigned) {
    vector<char> ncmd;
    if (!wstrtontbs(ncmd, in[0])) {
      wcerr << L"tglng: error: Encoding command \"" << in[0]
            << L"\": " << strerror(errno) << endl;
      return false;
    }
    char* argv[4] = {
      const_cast<char*>(getenv("SHELL")),
      const_cast<char*>("-c"),
      &ncmd[0],
      NULL,
    };
    if (!argv[0]) {
      //Set it to something reasonable and hope for the best
      argv[0] = const_cast<char*>("/bin/sh");
    }

    signed exitStatus;
    if (!invoke_external(out[0], exitStatus, argv, in[1])) {
      return false;
    }

    if (exitStatus && !parseBool(in[2])) {
      wcerr << L"tglng: error: Command \"" << in[0]
            << L"\" returned exit status " << exitStatus << endl;
      return false;
    }

    out[1] = intToStr(exitStatus);
    return true;
  }

  static GlobalBinding<TFunctionParser<2,3,cmdExec> > _exec(L"exec");
}
