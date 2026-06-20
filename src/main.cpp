#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  const std::unordered_set<std::string> builtins = {"echo", "exit", "type"};
  const std::string PATH = std::getenv("PATH");
  std::vector<std::string> paths;

  size_t start = 0;
  size_t end = PATH.find(':');

  while (end != std::string::npos) {
    paths.push_back(PATH.substr(start, end - start));
    start = end + 1;
    end = PATH.find(':', start);
  }
  paths.push_back(PATH.substr(start));

  while (true) {
    std::cout << "$ ";
    std::string input;
    std::getline(std::cin, input);
    size_t space = input.find(' ');
    std::string cmd = input.substr(0, space);
    std::string arg = (space != std::string::npos) ? input.substr(space + 1) : "";

    if (cmd == "exit") {
      break;
    } else if (cmd == "echo") {
      std::cout << arg << "\n";
    } else if (cmd == "type") {
      if (builtins.contains(arg)) {
        std::cout << std::format("{} is a shell builtin\n", arg);
      } else {
        bool found = false;
        for (auto& dir : paths) {
          fs::path full = fs::path(dir) / arg;
          if (fs::exists(full)) {
            auto perms = fs::status(full).permissions();
            if ((perms & fs::perms::owner_exec) != fs::perms::none) {
              std::cout << std::format("{} is {}\n", arg, full.string());
              found = true;
              break;
            }
          }
        }
        if (!found) std::cout << std::format("{}: not found\n", arg);
      }
    } else {
      std::cout << std::format("{}: command not found\n", cmd);
    }
  }
}
