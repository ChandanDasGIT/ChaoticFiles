#include "process/process.h"

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <poll.h>
#include <sstream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

constexpr std::size_t kMaxOutput = 16 * 1024 * 1024;

std::vector<char*> argv(const std::vector<std::string>& args)
{
    std::vector<char*> list;
    list.reserve(args.size() + 1);
    for (const std::string& arg : args) list.push_back(const_cast<char*>(arg.c_str()));
    list.push_back(nullptr);
    return list;
}

} // namespace

namespace Process {

std::string findProgram(const std::string& name)
{
    if (name.find('/') != std::string::npos) return access(name.c_str(), X_OK) == 0 ? name : std::string();
    const char* path = std::getenv("PATH");
    std::istringstream dirs(path ? path : "/usr/bin:/bin");
    std::string dir;
    while (std::getline(dirs, dir, ':')) {
        const std::string full = dir + "/" + name;
        struct stat info {};
        if (stat(full.c_str(), &info) == 0 && S_ISREG(info.st_mode) && access(full.c_str(), X_OK) == 0) return full;
    }
    return {};
}

Result run(const std::vector<std::string>& args, const std::function<bool()>& cancelled,
           const std::string& input, const std::string& workingDirectory)
{
    Result result;
    int in[2], out[2], err[2];
    if (pipe2(in, O_CLOEXEC) != 0) return result;
    if (pipe2(out, O_CLOEXEC) != 0) { close(in[0]); close(in[1]); return result; }
    if (pipe2(err, O_CLOEXEC) != 0) { close(in[0]); close(in[1]); close(out[0]); close(out[1]); return result; }

    auto list = argv(args);
    const pid_t pid = fork();
    if (pid == 0) {
        dup2(in[0], 0);
        dup2(out[1], 1);
        dup2(err[1], 2);
        if (!workingDirectory.empty() && chdir(workingDirectory.c_str()) != 0) _exit(127);
        setpgid(0, 0);
        execvp(list[0], list.data());
        _exit(127);
    }
    close(in[0]);
    close(out[1]);
    close(err[1]);
    if (pid < 0) {
        close(in[1]); close(out[0]); close(err[0]);
        return result;
    }

    if (!input.empty()) {
        signal(SIGPIPE, SIG_IGN);
        [[maybe_unused]] const auto written = write(in[1], input.data(), input.size());
    }
    close(in[1]);

    pollfd fds[2] = {{out[0], POLLIN, 0}, {err[0], POLLIN, 0}};
    char buffer[8192];
    int open = 2;
    while (open > 0) {
        const int ready = poll(fds, 2, 100);
        if (ready < 0 && errno != EINTR) break;
        if (cancelled && cancelled() && !result.cancelled) {
            result.cancelled = true;
            kill(-pid, SIGTERM);
            kill(pid, SIGTERM);
        }
        for (int i = 0; i < 2; ++i) {
            if (fds[i].fd < 0 || !(fds[i].revents & (POLLIN | POLLHUP))) continue;
            const ssize_t got = read(fds[i].fd, buffer, sizeof buffer);
            if (got <= 0) {
                close(fds[i].fd);
                fds[i].fd = -1;
                --open;
                continue;
            }
            std::string& target = i == 0 ? result.output : result.errors;
            if (target.size() < kMaxOutput) target.append(buffer, static_cast<std::size_t>(got));
        }
    }
    for (pollfd& fd : fds)
        if (fd.fd >= 0) close(fd.fd);

    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
    result.exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    result.started = result.exitCode != 127;
    return result;
}

bool startDetached(const std::vector<std::string>& args, const std::string& workingDirectory)
{
    int report[2];                                   // stays open until exec works; a byte means it failed
    if (pipe2(report, O_CLOEXEC) != 0) return false;

    auto list = argv(args);
    const pid_t pid = fork();
    if (pid == 0) {
        close(report[0]);
        if (fork() != 0) _exit(0);                   // the grandchild is not ours to wait for
        setsid();
        if (!workingDirectory.empty() && chdir(workingDirectory.c_str()) != 0) {}
        const int null = open("/dev/null", O_RDWR);
        if (null >= 0) {
            dup2(null, 0);
            dup2(null, 1);
            dup2(null, 2);
        }
        execvp(list[0], list.data());
        [[maybe_unused]] const auto written = write(report[1], "x", 1);
        _exit(127);
    }
    close(report[1]);
    if (pid < 0) {
        close(report[0]);
        return false;
    }
    while (waitpid(pid, nullptr, 0) < 0 && errno == EINTR) {}
    char failed = 0;
    const ssize_t got = read(report[0], &failed, 1);
    close(report[0]);
    return got == 0;
}

} // namespace Process
