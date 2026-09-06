#pragma once

#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#include <string>
#include <vector>

class Process
{
public:
    Process() = default;
    ~Process();

    bool alive() const;
    bool run(const std::vector<std::string>& args);
    bool stop();

private:
    pid_t _pid = -1;
};