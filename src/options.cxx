#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "options.hxx"

namespace tglng {
  std::wstring operationalFile(L"");
  bool implicitChdir = true;
  std::wstring userConfigs(L"");
  bool enableSystemConfig = true;
  std::list<std::wstring> scriptInputs;
  std::map<wchar_t,std::wstring> initialRegisters;
  bool dryRun = false;
  bool locateParseError = false;
}
