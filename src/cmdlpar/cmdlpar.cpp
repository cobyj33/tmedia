#include <tmedia/cmdlpar/cmdlpar.h>

#include <tmedia/util/defines.h>

#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <cctype>

#include <fmt/format.h>

namespace tmedia {

  inline bool is_shortopt(const char* arg) {
    return arg[0] == '-' && arg[1] != '-';
  }

  inline bool is_longopt(const char* arg) {
    return (arg[0] == '-' && arg[1] == '-') || arg[0] == ':';
  }

  struct CLIParseState {
    std::vector<CLIArg> pargs;
    const int argc;
    const char* const * const argv;
    int argi;
    CLIParseState(int argc, char** argv) : argc(argc), argv(argv), argi(1) {}
  };

  void cli_longopt_parse(CLIParseState& ps, const std::vector<std::string_view>& lopts_w_args) {
    int i = 0;
    const char * const longarg = ps.argv[ps.argi];
    CLIArg parg;
    parg.argi = ps.argi;
    parg.arg_type = CLIArgType::OPTION;

    if (longarg[0] == '-' && longarg[1] == '-') {
      parg.prefix = "--";
      i = 2;
    } else if (longarg[0] == ':') {
      parg.prefix = ":";
      i = 1;
    } else {
      throw std::runtime_error(fmt::format("[{}] Could not parse cli longopt "
      "prefix for arg: {}", FUNCDINFO, longarg));
    }

    const int parg_start = i;
    while (longarg[i] != '\0' && longarg[i] != '=') i++;
    parg.value = std::string_view(longarg + parg_start, i - parg_start);

    if (std::find(lopts_w_args.begin(), lopts_w_args.end(), parg.value) != lopts_w_args.end()) {
      bool defer_param = longarg[i] != '=';

      if (defer_param) { // read next arg as param
        if (ps.argi + 1 >= ps.argc) {
          throw std::runtime_error(fmt::format("[{}] param not found for long "
          "option {}.", FUNCDINFO, parg.value));
        }
        ps.argi++;
        parg.param = ps.argv[ps.argi];
      } else { // read param after equals sign
        i++; // consume equals
        const int param_start = i;
        while (longarg[i] != '\0') i++;
        parg.param = std::string_view(longarg + param_start, i - param_start);

        if (parg.param.length() == 0) {
          throw std::runtime_error(fmt::format("[{}] param not found for long "
          "option after '=' {}.", FUNCDINFO, longarg));
        }
      }
    } else { // this option should take no argument
      if (longarg[i] == '=') {
        throw std::runtime_error(fmt::format("[{}] Attempted to add param to "
        "non-param option: {}.", FUNCDINFO, longarg));
      }
    }

    ps.argi++; // consume current argument at index argi
    ps.pargs.push_back(parg);
  }

  void cli_shortopt_parse(CLIParseState& ps, std::string_view shortopts_with_args) {
    bool defer_param = false;
    const char * const shortarg = ps.argv[ps.argi];

    for (int j = 1; shortarg[j] != '\0'; j++) {
      char shortopt = shortarg[j];
      if (!std::isalpha(shortopt)) {
        throw std::runtime_error(fmt::format("[{}] shortopt must be an "
        "alphabetical character: {} ({})", FUNCDINFO, shortopt, shortarg));
      }

      ps.pargs.push_back(CLIArg(std::string_view(shortarg + j, 1), CLIArgType::OPTION, ps.argi, "-", ""));

      if (shortopts_with_args.find(shortopt) != std::string::npos) { // is a shortopt that takes an argument
        if (shortarg[j + 1] != '\0') // if it is not the last option in the option chain
          throw std::runtime_error(fmt::format("[{}] Could not "
          "parse short option requiring argument: {}.", FUNCDINFO, shortopt));
        defer_param = true;
      }
    }

    ps.argi++; // consume initial shortopt option string
    if (defer_param) {
      if (ps.argi >= ps.argc) {
        throw std::runtime_error(fmt::format("[{}] param not found for short "
        "option {}.", FUNCDINFO, ps.pargs[ps.pargs.size() - 1].value));
      }
      ps.pargs[ps.pargs.size() - 1].param = ps.argv[ps.argi];
      ps.argi++;
    }
  }

  std::vector<CLIArg> cli_parse(int argc, char** argv, std::string_view shortopts_with_args, const std::vector<std::string_view>& lopts_w_args) {
    CLIParseState ps(argc, argv);
    int since_opt_stopper = -1;

    while (ps.argi < argc) {
      since_opt_stopper += (since_opt_stopper >= 0) || (strcmp(argv[ps.argi], "--") == 0);

      if (since_opt_stopper >= 0) {
        if (since_opt_stopper > 0)
          ps.pargs.push_back(CLIArg(ps.argv[ps.argi], CLIArgType::POSITIONAL, ps.argi));
        ps.argi++;
      } else if (is_shortopt(ps.argv[ps.argi])) {
        cli_shortopt_parse(ps, shortopts_with_args);
      } else if (is_longopt(ps.argv[ps.argi])) {
        cli_longopt_parse(ps, lopts_w_args);
      } else {
        ps.pargs.push_back(CLIArg(ps.argv[ps.argi], CLIArgType::POSITIONAL, ps.argi));
        ps.argi++;
      }
    }

    return ps.pargs;
  }
}
