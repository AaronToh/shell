#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <span>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_set>
#include <utility>
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

  const std::unordered_set<std::string> builtins = {"cd", "echo", "exit", "jobs", "pwd", "type"};
  const std::string PATH = std::getenv("PATH");
  std::vector<std::string> paths;
  std::vector<std::pair<pid_t, std::string>> backgroundJobs = {{}};

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
    size_t i = 0;
    std::vector<std::string> args;
    std::string arg = "";
    while (i < input.size()) {
      if (input[i] == ' ') {
        if (arg.size() > 0) {
          args.push_back(arg);
          arg = "";
        }
        i++;
      } else if (input[i] == '\\') {
        i++; // assume character exists after backslash
        arg += input[i];
        i++;
      } else if (input[i] == '\'') {
        size_t closing = input.find('\'', i + 1); // assume that closing quote exists
        arg += input.substr(i + 1, closing - i - 1);
        i = closing + 1;
      } else if (input[i] == '\"') {
        i++;
        while (input[i] != '\"') { // assume that closing quote exists
          if (input[i] == '\\') {
            if (input[i+1] == '\"' || input[i+1] == '\\') i++;
          }
          arg += input[i];
          i++;
        }
        i++;
      } else {
        arg += input[i];
        i++;
      }
    }
    if (arg.size() > 0) args.push_back(arg);
    if (args.size() == 0) continue;
    std::string cmd = args[0];
    bool isBackground = false;
    if (args.back() == "&") {
      isBackground = true;
      args.pop_back();
    }

    if (cmd == "cd") {
      std::string arg = args[1]; // assume that path given
      if (arg == "~" || arg.substr(0, 2) == "~/") {
        std::string home = std::getenv("HOME");
        arg = home + arg.substr(1);
      }
      if (fs::exists(arg)) fs::current_path(arg);
      else std::cout << std::format("cd: {}: No such file or directory\n", arg);
    } else if (cmd == "echo") {
      std::string msg = "";
      std::span<std::string> sliced(args.begin() + 1, args.end());
      for (auto& arg : sliced) {
        msg += (msg.empty() ? "" : " ") + arg;
      }
      std::cout << msg << "\n";
    } else if (cmd == "exit") {
      break;
    } else if (cmd == "jobs") {
      for (size_t id = 1; id < backgroundJobs.size(); id++) {
        auto& [pid, input] = backgroundJobs[id];
        std::string status;
        size_t last = backgroundJobs.size() - 1;
        char marker = id == last ? '+' : (id == last - 1 ? '-' : ' ');
        int s;
        pid_t result = waitpid(pid, &s, WNOHANG);
        
        if (result == 0) {
          status = "Running";
        } else {
          if (WIFEXITED(s)) status = "Done";
        }

        std::cout << std::format("[{}]{}  {:<24}{} &\n", id, marker, status, input);
        if (status == "Done") backgroundJobs.pop_back(); // assume if done it is the last one
      }
    } else if (cmd == "pwd") {
      std::cout << std::format("{}\n", fs::current_path().string());
    } else if (cmd == "type") {
      std::string arg = args[1]; // assumes one arg given
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
        std::vector<char*> argv;
        for (auto& arg : args) {
          argv.push_back(arg.data());
        }
        argv.push_back(nullptr);

        pid_t pid = fork();
        if (pid == 0) {
          execv(full.c_str(), argv.data());
          _exit(1);
        } else {
          if (isBackground) {
            backgroundJobs.push_back({pid, input.substr(0, input.rfind(" &"))});
            std::cout << std::format("[{}] {}\n", backgroundJobs.size() - 1, pid);
          }
          else waitpid(pid, nullptr, 0);
        }
      }
    }
  }
}
