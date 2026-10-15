#pragma once

#include <sys/types.h>

#include <string>
#include <utility>
#include <vector>

class Process
{
public:
    using Environment = std::vector<std::pair<std::string, std::string>>;

    Process() = default;
    ~Process();

    Process(const Process &) = delete;
    Process &operator=(const Process &) = delete;

    Process(Process &&other) noexcept;
    Process &operator=(Process &&other) noexcept;

    bool alive() const;

    bool run(
        const std::vector<std::string> &args,
        const Environment &env = {});

    bool stop();

private:
    static void wait_for(pid_t pid) noexcept;

    mutable pid_t _pid = -1;
};
