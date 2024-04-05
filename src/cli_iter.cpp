#include "cli_iter.h"

#include "funcmac.h"

#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <cctype>

#include <fmt/format.h>

namespace tmedia {

  bool is_shortopt(char* arg) {
    return arg[0] == '-' && arg[1] != '-';
  }

  bool is_longopt(char* arg) {
    return (arg[0] == '-' && arg[1] == '-') || arg[0] == ':';
  }

  struct LongOptParseRes {
    CLIArg arg;
    int nextIndex;
  };

  LongOptParseRes cli_longopt_parse(int argc, char** argv, int index, const std::vector<std::string>& longopts_with_args) {
    LongOptParseRes res;
    res.nextIndex = index + 1;
    res.arg.arg_type = CLIArgType::OPTION;

    int i = 0;

    if (argv[index][0] == '-' && argv[index][1] == '-') {
      res.arg.prefix = "--";
      i = 2;
    } else if (argv[index][0] == ':') {
      res.arg.prefix = ":";
      i = 1;
    } else {
      throw std::runtime_error(fmt::format("[{}] Could not parse cli longopt "
      "prefix for arg: {}", FUNCDINFO, argv[index]));
    }

    for (; argv[index][i] != '\0' && argv[index][i] != '='; i++) {
      res.arg.value += argv[index][i];
    }

    if (std::find(longopts_with_args.begin(), longopts_with_args.end(), res.arg.value) != longopts_with_args.end()) {
      bool defer_param = argv[index][i] != '=';
      
      if (defer_param) { // read next arg as param
        if (res.nextIndex >= argc) {
          throw std::runtime_error(fmt::format("[{}] param not found for long "
          "option {}.", FUNCDINFO, res.arg.value));
        }

        res.arg.param = argv[res.nextIndex++];
      } else { // read param after equals sign
        
        for (i++; argv[index][i] != '\0'; i++) {
          res.arg.param += argv[index][i];
        }

        if (res.arg.param.length() == 0) {
          throw std::runtime_error(fmt::format("[{}] param not found for long "
          "option after '=' {}.", FUNCDINFO, argv[index]));
        }
      }
    } else {
      if (argv[index][i] == '=') {
        throw std::runtime_error(fmt::format("[{}] Attempted to add param to "
        "non-param option: {}.", FUNCDINFO, argv[index]));
      }
    }

    return res;
  }

  struct ShortOptParseRes {
    std::vector<CLIArg> args;
    int nextIndex;  
  };

  ShortOptParseRes cli_shortopt_parse(int argc, char** argv, int index, std::string_view shortopts_with_args) {
    ShortOptParseRes res;
    res.nextIndex = index + 1;
    bool defer_param = false;

    for (int j = 1; argv[index][j] != '\0'; j++) {
      char shortopt = argv[index][j];
      if (!std::isalpha(shortopt)) {
        throw std::runtime_error(fmt::format("[{}] shortopt must be an "
        "alphabetical character: {} ({})", FUNCDINFO, shortopt, argv[index]));
      }
      
      CLIArg arg(std::string(1, shortopt), CLIArgType::OPTION, "-", "");

      res.args.push_back(arg);
      if (shortopts_with_args.find(shortopt) != std::string::npos) {
        defer_param = argv[index][j + 1] == '\0';
        if (!defer_param) throw std::runtime_error(fmt::format("[{}] Could not "
        "parse short option requiring argument: {}.", FUNCDINFO, shortopt));
      }
    }

    if (defer_param) {
      if (res.nextIndex >= argc) {
        throw std::runtime_error(fmt::format("[{}] param not found for short "
        "option {}.", FUNCDINFO, res.args[res.args.size() - 1].value));
      }
      res.args[res.args.size() - 1].param = argv[res.nextIndex++];
    }

    return res;
  }

  CLIArg cli_create_pos_arg(char* arg) {
    CLIArg posarg;
    posarg.value = arg;
    posarg.arg_type = CLIArgType::POSITIONAL;
    return posarg;
  }

  std::vector<CLIArg> cli_parse(int argc, char** argv, std::string_view shortopts_with_args, const std::vector<std::string>& longopts_with_args) {
    std::vector<CLIArg> args;
    int since_opt_stopper = -1;
    
    int i = 1;
    while (i < argc) {
      since_opt_stopper += (since_opt_stopper >= 0) || (strcmp(argv[i], "--") == 0);

      if (since_opt_stopper >= 0) {
        if (since_opt_stopper > 0)
          args.push_back(cli_create_pos_arg(argv[i]));
        i++;
      } else if (is_shortopt(argv[i])) {
        ShortOptParseRes res = cli_shortopt_parse(argc, argv, i, shortopts_with_args);
        for (const CLIArg& arg : res.args) {
          args.push_back(arg);
        }

        i = res.nextIndex;
      } else if (is_longopt(argv[i])) {
        LongOptParseRes res = cli_longopt_parse(argc, argv, i, longopts_with_args);
        args.push_back(res.arg);
        i = res.nextIndex;
      } else {
        args.push_back(cli_create_pos_arg(argv[i++]));
      }
    }

    return args;
  }

}
