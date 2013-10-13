#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "options.hxx"

namespace tglng {
  std::string operationalFile("");
  bool implicitChdir = true;
  std::list<std::string> userConfigs;
  bool enableSystemConfig = true;
  std::list<std::string> scriptInputs;
  std::map<wchar_t,std::wstring> initialRegisters;
  bool dryRun = false;
  bool locateParseError = false;
}
