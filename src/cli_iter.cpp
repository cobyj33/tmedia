#include "cli_iter.h"

#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <cctype>

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

  struct LongOptPrefixConsumeRes {
    int nextIndex;
    std::string prefix;
  };

  LongOptPrefixConsumeRes cli_longopt_prefix_consume(char* arg) {
    LongOptPrefixConsumeRes res;
    if (arg[0] == '-' && arg[1] == '-') {
      res.prefix = "--";
      res.nextIndex = 2;
    }
    else if (arg[0] == ':') {
      res.prefix = ":";
      res.nextIndex = 1;
    } else {
      throw std::runtime_error("[cli_longopt_prefix_consume] Could not parse cli longopt prefix for arg: " + std::string(arg));
    }

    return res;
  }

  LongOptParseRes cli_longopt_parse(int argc, char** argv, int index, std::vector<std::string> longopts_with_args) {
    LongOptParseRes res;
    res.nextIndex = index + 1;
    res.arg.arg_type = CLIArgType::OPTION;
    bool defer_param = true;

    int i = 0;

    if (argv[index][0] == '-' && argv[index][1] == '-') {
      res.arg.prefix = "--";
      i = 2;
    } else if (argv[index][0] == ':') {
      res.arg.prefix = ":";
      i = 1;
    } else {
      throw std::runtime_error("[cli_longopt_prefix_consume] Could not parse cli longopt prefix for arg: " + std::string(argv[index]));
    }

    for (; argv[index][i] != '\0' && argv[index][i] != '='; i++) {
      res.arg.value += argv[index][i];
    }

    if (std::find(longopts_with_args.begin(), longopts_with_args.end(), res.arg.value) != longopts_with_args.end()) {
      defer_param = argv[index][i] != '=';
      
      if (defer_param) { // read next arg as param
        if (res.nextIndex >= argc) {
          throw std::runtime_error("[cli_longopt_parse] param not found for long option " + res.arg.value);
        }

        res.arg.param = argv[res.nextIndex++];
      } else { // read param after equals sign
        
        for (i++; argv[index][i] != '\0'; i++) {
          res.arg.param += argv[index][i];
        }

        if (res.arg.param.length() == 0) {
          throw std::runtime_error("[cli_longopt_parse] param not found for long option after '=' " + std::string(argv[index]));
        }

      }

    } else {
      if (argv[index][i] == '=') {
        throw std::runtime_error("[cli_longopt_parse] Attempted to add param to non-param option: " + std::string(argv[index]));
      }
    }

    return res;
  }

  struct ShortOptParseRes {
    std::vector<CLIArg> args;
    int nextIndex;  
  };

  ShortOptParseRes cli_shortopt_parse(int argc, char** argv, int index, std::string shortopts_with_args) {
    ShortOptParseRes res;
    res.nextIndex = index + 1;
    bool defer_param = false;

    for (int j = 1; argv[index][j] != '\0'; j++) {
      char shortopt = argv[index][j];
      if (!std::isalpha(shortopt)) {
        throw std::runtime_error(std::string("[cli_shortopt_parse] shortopt must be an alphabetical character: ") + shortopt + " (" + std::string(argv[index]) + ")");
      }

      CLIArg arg;
      arg.value = shortopt;
      arg.arg_type = CLIArgType::OPTION;
      arg.prefix = "-";

      res.args.push_back(arg);
      if (shortopts_with_args.find(shortopt) != std::string::npos) {
        defer_param = argv[index][j + 1] == '\0';
        if (!defer_param) throw std::runtime_error("[cli_shortopt_parse] Could not parse short option requiring argument: " + shortopt);
      }
    }

    if (defer_param) {
      if (res.nextIndex >= argc) {
        throw std::runtime_error("[cli_longopt_parse] param not found for short option " + res.args[res.args.size() - 1].value);
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

  std::vector<CLIArg> cli_parse(int argc, char** argv, std::string shortopts_with_args, std::vector<std::string> longopts_with_args) {
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
