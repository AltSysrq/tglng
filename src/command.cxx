#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "command.hxx"

namespace tglng {
  Command::Command(Command* left_)
  : left(left_)
  { }

  Command::~Command() {
    if (left)
      delete left;
  }
}
