#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

std::string findInPath(const std::string& arg, const std::vector<std::string>& paths) {
  for (const auto& dir : paths) {
    fs::path full = fs::path(dir) / arg;
    if (fs::exists(full)) {
      auto perms = fs::status(full).permissions();
      if ((perms & fs::perms::owner_exec) != fs::perms::none) {
        return full.string();
      }
    }
  }
  return "";
}

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  const std::unordered_set<std::string> builtins = {"cd", "echo", "exit", "pwd", "type"};
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

    if (cmd == "cd") {
      if (fs::exists(arg)) fs::current_path(arg);
      else std::cout << std::format("cd: {}: No such file or directory\n", arg);
    } else if (cmd == "echo") {
      std::cout << arg << "\n";
    } else if (cmd == "exit") {
      break;
    } else if (cmd == "pwd") {
      std::cout << std::format("{}\n", fs::current_path().string());
    } else if (cmd == "type") {
      if (builtins.contains(arg)) {
        std::cout << std::format("{} is a shell builtin\n", arg);
      } else {
        std::string full = findInPath(arg, paths);
        if (full.empty()) std::cout << std::format("{}: not found\n", arg);
        else std::cout << std::format("{} is {}\n", arg, full);
      }
    } else {
      std::string full = findInPath(cmd, paths);
      if (full.empty()) std::cout << std::format("{}: command not found\n", cmd);
      else {
        std::vector<std::string> argt;
        argt.push_back(cmd);
        size_t start = 0;
        size_t end = arg.find(' ');
        if (!arg.empty()) {
          while (end != std::string::npos) {
            argt.push_back(arg.substr(start, end - start));
            start = end + 1;
            end = arg.find(' ', start);
          }
          argt.push_back(arg.substr(start));
        }

        std::vector<char*> argv;
        for (auto& p : argt) {
          argv.push_back(const_cast<char*>(p.c_str()));
        }
        argv.push_back(nullptr);

        pid_t pid = fork();
        if (pid == 0) {
          execv(full.c_str(), argv.data());
        } else {
          wait(nullptr);
        }
      }
    }
  }
}
