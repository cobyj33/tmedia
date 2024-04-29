Here are just some general steps, which may help to remind someone where
they should try to update documentation and metadata whenever a change has
been made.



## If any new controls have been added:

- Make sure to update the TMEDIA_CONTROLS_USAGE string in src/tmedia_cli.cpp to
  document the new control.
- Make sure to update the "\# TMedia Controls" section in README.md to document
  the new control


## If any new command line options have been added:

- Make sure to update the TMEDIA_CLI_ARGS_DESC string in src/tmedia_cli.cpp to
  document the new CLI Argument.
- Make sure to update the "\# CLI Arguments" section in README.md to document
  the new CLI Argument.
- Make sure to update the "CLI Arguments" section in doc/cli.md to document
  the new CLI Argument.
