#include <iostream>
#include <string>

#include "base/StringUtils.h"
#include "configure/Default.h"

using std::string;

int main(int argc, char **argv) {
  int ret = 0;

  string lowercaseProtocol = "HEllo wOrld";
  for (string::iterator pc = lowercaseProtocol.begin();
       pc != lowercaseProtocol.end(); ++pc) {
    *pc = std::tolower(*pc);
  }

  std::cout <<lowercaseProtocol << std::endl;

  std::cout << QS::StringUtils::ToLower("I LOVe you") << std::endl;
  std::cout << QS::StringUtils::ToUpper("I LOVe you") << std::endl;

  std::cout << "default port for http: " << QS::Configure::Default::GetDefaultPort("http")<<std::endl;

  return ret;
}
