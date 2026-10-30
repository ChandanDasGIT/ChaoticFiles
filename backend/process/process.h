#pragma once
#include <functional>
#include <string>
#include <vector>

// Other programs. Nothing here uses a shell, so a file name can never be read as a command.
namespace Process {

struct Result {
    bool started = false;         // false if the program could not be run at all
    bool cancelled = false;
    int exitCode = -1;
    std::string output;           // what it wrote to standard output
    std::string errors;           // what it wrote to standard error
};

std::string findProgram(const std::string& name);    // its full path, or empty

// Runs a program, waits for it and collects its output. `cancelled`, if given, stops it when it returns true.
// `input` is written to its standard input.
Result run(const std::vector<std::string>& args, const std::function<bool()>& cancelled = {},
           const std::string& input = {}, const std::string& workingDirectory = {});

// Starts a program that lives on by itself, for example an app. Returns false if it could not start.
bool startDetached(const std::vector<std::string>& args, const std::string& workingDirectory = {});

} // namespace Process
