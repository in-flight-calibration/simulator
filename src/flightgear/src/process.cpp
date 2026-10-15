#include "process.hpp"

#include <spawn.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>

extern char **environ;

Process::~Process()
{
    wait_for(_pid);
}

Process::Process(Process &&other) noexcept
    : _pid(other._pid)
{
    other._pid = -1;
}

Process &Process::operator=(Process &&other) noexcept
{
    if (this != &other) {
        if (alive()) {
            stop();
            wait_for(_pid);
            _pid = -1;
        }

        _pid = other._pid;
        other._pid = -1;
    }

    return *this;
}

void Process::wait_for(pid_t pid) noexcept
{
    if (pid <= 0) {
        return;
    }

    while (waitpid(pid, nullptr, 0) < 0 && errno == EINTR) {
    }
}

bool Process::alive() const
{
    if (_pid <= 0) {
        return false;
    }

    int status = 0;
    pid_t result;

    do {
        result = waitpid(_pid, &status, WNOHANG);
    } while (result < 0 && errno == EINTR);

    if (result == _pid || (result < 0 && errno == ECHILD)) {
        _pid = -1;
        return false;
    }

    return result == 0;
}

bool Process::run(
    const std::vector<std::string> &args,
    const Environment &env)
{
    if (args.empty() || args.front().empty() || alive()) {
        return false;
    }

    // Copy the current environment.
    std::vector<std::string> env_storage;

    for (char **entry = environ; entry != nullptr && *entry != nullptr; ++entry) {
        env_storage.emplace_back(*entry);
    }

    // Override existing variables or add new ones.
    for (const auto &[key, value] : env) {
        if (key.empty()
            || key.find('=') != std::string::npos
            || key.find('\0') != std::string::npos
            || value.find('\0') != std::string::npos) {
            return false;
        }

        const std::string prefix = key + "=";

        const auto new_end = std::remove_if(
            env_storage.begin(),
            env_storage.end(),
            [&prefix](const std::string &entry) {
                return entry.compare(0, prefix.size(), prefix) == 0;
            });

        env_storage.erase(new_end, env_storage.end());
        env_storage.push_back(prefix + value);
    }

    // Build argv.
    std::vector<char *> argv;
    argv.reserve(args.size() + 1);

    for (const auto &arg : args) {
        argv.push_back(const_cast<char *>(arg.c_str()));
    }

    argv.push_back(nullptr);

    // Build envp.
    std::vector<char *> envp;
    envp.reserve(env_storage.size() + 1);

    for (auto &entry : env_storage) {
        envp.push_back(const_cast<char *>(entry.c_str()));
    }

    envp.push_back(nullptr);

    // Spawn the process with its own environment.
    pid_t child_pid = -1;

    const int result = posix_spawnp(
        &child_pid,
        argv.front(),
        nullptr,
        nullptr,
        argv.data(),
        envp.data());

    if (result != 0) {
        return false;
    }

    _pid = child_pid;
    return true;
}

bool Process::stop()
{
    if (!alive()) {
        return false;
    }

    return kill(_pid, SIGINT) == 0;
}