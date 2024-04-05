#ifndef TMEDIA_CLI_ITER_H
#define TMEDIA_CLI_ITER_H

#include <string>
#include <string_view>
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
    CLIArg() {}
    CLIArg(std::string_view value, CLIArgType arg_type, std::string prefix, std::string param) : value(value), arg_type(arg_type), prefix(prefix), param(param) {}
  };

  std::vector<CLIArg> cli_parse(int argc, char** argv, std::string_view shortopts_with_args, const std::vector<std::string>& longopts_with_args);
};


#endif