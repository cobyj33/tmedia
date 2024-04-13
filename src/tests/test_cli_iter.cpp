#include <tmedia/cli/cli_iter.h>

#include <catch2/catch_test_macros.hpp>

#include <vector>
#include <utility>
#include <string_view>
#include <string>
#include <cstddef>

class MockCLI {
  public:
    int argc;
    char** argv;

  MockCLI(const std::vector<std::string_view>& cli_args) {
    this->argc = static_cast<int>(cli_args.size());

    this->argv = new char*[this->argc + 1];
    for (int i = 0; i < this->argc; i++) {
      std::size_t istrlen = cli_args[i].length();
      this->argv[i] = new char[istrlen + 1];

      for (std::size_t j = 0; j < istrlen; j++) {
        this->argv[i][j] = cli_args[i][j];
      }
      this->argv[i][istrlen] = '\0';
    }
    this->argv[argc] = nullptr;
  }

  ~MockCLI() {
    for (int i = 0; i < this->argc; i++) {
      delete[] this->argv[i];
    }
    delete[] this->argv;
  }
};

TEST_CASE("cli_iter", "[cli_iter]") {

  SECTION("Should Succeed") {
    SECTION("stringed shortopts") {
      MockCLI test1({"./tmedia", "-ab", "-c"});
      std::vector<tmedia::CLIArg> test1res = tmedia::cli_parse(test1.argc, test1.argv, "", {});
      
      REQUIRE(test1res.size() == 3UL);
      REQUIRE(test1res[0].value == "a");
      REQUIRE(test1res[0].prefix == "-");
      REQUIRE(test1res[0].arg_type == tmedia::CLIArgType::OPTION);

      REQUIRE(test1res[1].value == "b");
      REQUIRE(test1res[1].prefix == "-");
      REQUIRE(test1res[1].arg_type == tmedia::CLIArgType::OPTION);

      REQUIRE(test1res[2].value == "c");
      REQUIRE(test1res[2].prefix == "-");
      REQUIRE(test1res[2].arg_type == tmedia::CLIArgType::OPTION);
    }

    SECTION("Colon Prefix") {
      MockCLI mockcli({"./tmedia", "./E01.mp4", ":ignore-audio", "--ignore-video"});
      std::vector<tmedia::CLIArg> parsed = tmedia::cli_parse(mockcli.argc, mockcli.argv, "", {});

      REQUIRE(parsed.size() == 3UL);
      REQUIRE(parsed[0].arg_type == tmedia::CLIArgType::POSITIONAL);
      REQUIRE(parsed[0].value == "./E01.mp4");

      REQUIRE(parsed[1].arg_type == tmedia::CLIArgType::OPTION);
      REQUIRE(parsed[1].value == "ignore-audio");
      REQUIRE(parsed[1].prefix == ":");

      REQUIRE(parsed[2].arg_type == tmedia::CLIArgType::OPTION);
      REQUIRE(parsed[2].value == "ignore-video");
      REQUIRE(parsed[2].prefix == "--");
    }

    SECTION("opt stopper") {
      MockCLI mockcli({"./tmedia", "./E01.mp4", ":ignore-audio", "--", "--ignore-video", "--"});
      std::vector<tmedia::CLIArg> parsed = tmedia::cli_parse(mockcli.argc, mockcli.argv, "", {});

      REQUIRE(parsed.size() == 4UL);
      REQUIRE(parsed[0].arg_type == tmedia::CLIArgType::POSITIONAL);
      REQUIRE(parsed[0].value == "./E01.mp4");

      REQUIRE(parsed[1].arg_type == tmedia::CLIArgType::OPTION);
      REQUIRE(parsed[1].value == "ignore-audio");
      REQUIRE(parsed[1].prefix == ":");

      REQUIRE(parsed[2].arg_type == tmedia::CLIArgType::POSITIONAL);
      REQUIRE(parsed[2].value == "--ignore-video");
      REQUIRE(parsed[2].prefix == "");

      REQUIRE(parsed[3].arg_type == tmedia::CLIArgType::POSITIONAL);
      REQUIRE(parsed[3].value == "--");
    }

    SECTION("equals opts") {
      MockCLI mockcli({"./tmedia", "./E01.mp4", "--volume=50%"});
      std::vector<tmedia::CLIArg> parsed = tmedia::cli_parse(mockcli.argc, mockcli.argv, "", {"volume"});

      REQUIRE(parsed.size() == 2UL);
      REQUIRE(parsed[0].arg_type == tmedia::CLIArgType::POSITIONAL);
      REQUIRE(parsed[0].value == "./E01.mp4");

      REQUIRE(parsed[1].arg_type == tmedia::CLIArgType::OPTION);
      REQUIRE(parsed[1].value == "volume");
      REQUIRE(parsed[1].prefix == "--");
      REQUIRE(parsed[1].param == "50%");
    }

    SECTION("equals opts") {
      MockCLI mockcli({"./tmedia", "./E01.mp4", "--volume=50%"});
      std::vector<tmedia::CLIArg> parsed = tmedia::cli_parse(mockcli.argc, mockcli.argv, "", {"volume"});

      REQUIRE(parsed.size() == 2UL);
      REQUIRE(parsed[0].arg_type == tmedia::CLIArgType::POSITIONAL);
      REQUIRE(parsed[0].value == "./E01.mp4");

      REQUIRE(parsed[1].arg_type == tmedia::CLIArgType::OPTION);
      REQUIRE(parsed[1].value == "volume");
      REQUIRE(parsed[1].prefix == "--");
      REQUIRE(parsed[1].param == "50%");
    }

    SECTION("deferred longopt") {
      MockCLI mockcli({"./tmedia", "./E01.mp4", "--loop-type", "repeat"});
      std::vector<tmedia::CLIArg> parsed = tmedia::cli_parse(mockcli.argc, mockcli.argv, "", {"loop-type"});

      REQUIRE(parsed.size() == 2UL);
      REQUIRE(parsed[0].arg_type == tmedia::CLIArgType::POSITIONAL);
      REQUIRE(parsed[0].value == "./E01.mp4");

      REQUIRE(parsed[1].arg_type == tmedia::CLIArgType::OPTION);
      REQUIRE(parsed[1].value == "loop-type");
      REQUIRE(parsed[1].prefix == "--");
      REQUIRE(parsed[1].param == "repeat");
    }
  } // Should Succeed

  SECTION("Should Fail") {
    
    SECTION("stringed arg opt not at end") {
      MockCLI mockcli({"./tmedia", "-abcde", "somearg"});
      REQUIRE_THROWS(tmedia::cli_parse(mockcli.argc, mockcli.argv, "a", {}));
      REQUIRE_THROWS(tmedia::cli_parse(mockcli.argc, mockcli.argv, "b", {}));
      REQUIRE_THROWS(tmedia::cli_parse(mockcli.argc, mockcli.argv, "c", {}));
      REQUIRE_THROWS(tmedia::cli_parse(mockcli.argc, mockcli.argv, "d", {}));
      REQUIRE_NOTHROW(tmedia::cli_parse(mockcli.argc, mockcli.argv, "e", {}));
      std::vector<tmedia::CLIArg> parsed = tmedia::cli_parse(mockcli.argc, mockcli.argv, "e", {});
      REQUIRE(parsed[4].param == "somearg");
    }

    SECTION("equals opts") {
      MockCLI mockcli({"./tmedia", "./E01.mp4", "--volume=50%"});
      REQUIRE_THROWS(tmedia::cli_parse(mockcli.argc, mockcli.argv, "", {}));
      REQUIRE_NOTHROW(tmedia::cli_parse(mockcli.argc, mockcli.argv, "", {"volume"}));
    }

    SECTION("equals in shortopt") {
      MockCLI mockcli({"./tmedia", "./E01.mp4", "-l=repeat-one"});
      REQUIRE_THROWS(tmedia::cli_parse(mockcli.argc, mockcli.argv, "", {}));
      REQUIRE_THROWS(tmedia::cli_parse(mockcli.argc, mockcli.argv, "l", {}));
    }
  }

}