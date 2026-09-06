#include "process.hpp"

Process::~Process()
{
    if (_pid > 0) {
        waitpid(_pid, nullptr, 0);
    }
}

bool Process::alive() const
{
    return _pid > 0 && waitpid(_pid, nullptr, WNOHANG) == 0;
}

bool Process::run(const std::vector<std::string> &args)
{
    if (args.empty()) {
        return false;
    }

    _pid = fork();

    if (_pid < 0) {
        return false;
    }

    if (_pid == 0) {
        std::vector<char*> argv;

        for (auto& arg : args) {                
            argv.push_back(const_cast<char*>(arg.c_str()));
        }

        argv.push_back(nullptr);
        execvp(argv[0], argv.data());
        _exit(127);
    }

    return true;
}

bool Process::stop()
{
    if (alive()) {
        kill(_pid, SIGINT);
        return true;
    }

    return false;
}
