#ifndef TMEDIA_CMDLPAR_H
#define TMEDIA_CMDLPAR_H

#include <string>
#include <string_view>
#include <vector>

namespace tmedia {

  /**
   * I decided to spin my own simple CLI argument parser, since I had wanted to
   * add arguments whose meaning changes depending on what media sources are
   * defined before them, like VLC's ":" arguments and mpv's "{ }" grouping
   * symbols. This meant that I have to have sequential access to arguments and
   * their meanings, but not much more.
   * 
   * I decided to make the general parser as simple as possible, and
   * the actual semantics and effects for each option are present at a
   * higher level. However, cli_parse works well to classify which parts
   * of the command line input are positional arguments, which parts are 
   * options, and which parts are parameters for options. Which is all it needs
   * to do.
   * 
   * Also, empty parameters are not supported. That isn't a problem for tmedia,
   * but it should definitely be noted.
   * 
   * Most of the syntax of parsing is shared with getopt_long,
   * where "--" declares all cli arguments afterward to be positional arguments,
   * and long options can have parameters declared as "--argument=parameter"
   * or "--argument parameter".
   * 
   * Note that this current implementation is set to also accept the prefix ":"
   * for tmedia's purposes!
  */

  enum class CLIArgType {
    POSITIONAL,
    OPTION
  };

  /**
   * A representation for a parsed CLI Argument. Each argument is either a
   * positional argument or a Option (Not to be confused with an optional
   * argument). The type of a CLIArg can be deduced from the arg_type member.
   * 
   * If arg_type == CLIArgType::POSITIONAL, then "value" contains the full 
   * string of the positional argument on the command line.
   * 
   * If arg_type == CLIArgType::OPTION, then "value" is the name of the option,
   * "prefix" is the prefix used to declare that option ("-", "--", or ":",
   * although more can be added), and "param" is the parameter given to the
   * option, if there was any.
  */
  struct CLIArg {
    std::string_view value;
    CLIArgType arg_type;
    int argi;
    std::string_view prefix;
    std::string_view param;
    CLIArg() {}
    CLIArg(std::string_view value, CLIArgType arg_type, int argi, std::string_view prefix, std::string_view param) : value(value), arg_type(arg_type), argi(argi), prefix(prefix), param(param) {}
    CLIArg(std::string_view value, CLIArgType arg_type, int argi) : value(value), arg_type(arg_type), argi(argi), prefix(""), param("") {}
    CLIArg(const CLIArg& o) : value(o.value), arg_type(o.arg_type), argi(o.argi), prefix(o.prefix), param(o.param) {}
    CLIArg(CLIArg&& o) : value(o.value), arg_type(o.arg_type), argi(o.argi), prefix(o.prefix), param(o.param) {}
  };

  /**
   * Parse and structure cli commands in order.
   * 
   * @param shortopts_with_args a string of all short options which should be
   * parsed to accept a string parameter
   * @param longopts_with_args a vector of long options which should be parsed
   * to accept some sort of parameter
  */
  std::vector<CLIArg> cli_parse(int argc, char** argv, std::string_view shortopts_with_args, const std::vector<std::string_view>& longopts_with_args);

  /**
   * notes:
   * 
   * At first, tmedia had used [p-ranav/argparse](https://github.com/p-ranav/argparse)
   * 
   * I decided to spin my own simple CLI argument parser, since I had wanted to
   * add arguments whose meaning changes depending on what media sources are
   * defined before them, like VLC's ":" arguments and mpv's "{ }" grouping
   * symbols. This meant that I have to have sequential access to arguments and
   * their meanings, but not much more.
   * 
   * 
   * I also thought that the getopt style of iterating over the passed in argc
   * and argv was somewhat overkill. While in a general setting, I could see
   * wanting to avoid the memory allocation, I really don't care if a few
   * allocations are made, since tmedia makes allocations literally all the
   * time.
   * 
   * As of right now, I don't see why optional arguments would actually be a
   * good thing for most CLI apps, so I decided to not worry about them for now.
  */
};


#endif