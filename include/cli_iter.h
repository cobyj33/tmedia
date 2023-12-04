#ifndef TMEDIA_CLI_ITER_H
#define TMEDIA_CLI_ITER_H

#include <string>
#include <vector>

namespace tmedia {
  enum class CLIArgType {
    POSITIONAL,
    OPTION
  };

  struct CLIArg {
    std::string value;
    CLIArgType arg_type;
    
    std::string prefix;
    std::string param;
  };

  std::vector<CLIArg> cli_parse(int argc, char** argv, std::string shortopts_with_args, std::vector<std::string> longopts_with_args);
};


#endif