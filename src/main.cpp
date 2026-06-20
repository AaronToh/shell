#include <format>
#include <iostream>
#include <string>
#include <unordered_set>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  const std::unordered_set<std::string> builtins = {"echo", "exit", "type"};

  while (true) {
    std::cout << "$ ";
    std::string input;
    std::getline(std::cin, input);
    size_t space = input.find(' ')
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
        std::cout << std::format("{}: not found\n", arg);
      }
    } else {
      std::cout << std::format("{}: command not found\n", cmd);
    }
  }
}
