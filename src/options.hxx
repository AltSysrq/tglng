#ifndef OPTIONS_HXX_
#define OPTIONS_HXX_

#include <string>
#include <list>
#include <map>

namespace tglng {
  extern std::wstring operationalFile;
  extern bool implicitChdir;
  extern std::wstring userConfigs;
  extern bool enableSystemConfig;
  extern std::list<std::wstring> scriptInputs;
  extern std::map<wchar_t,std::wstring> initialRegisters;
  extern bool dryRun;
  extern bool locateParseError;
}

#endif /* OPTIONS_HXX_ */
