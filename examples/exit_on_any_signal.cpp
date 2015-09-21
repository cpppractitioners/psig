#include <psig/psig.hpp>
#include <iostream>

bool handle_signal(int sig)
{
    std::cout << "Handling signal " << sig << std::endl;

    switch (sig)
    {
        case SIGTERM:
        case SIGHUP:
            return false;
        case SIGINT:
        default:
            return true;
    }
}

int handle_exit()
{
    std::cout << "Handling exit" << std::endl;
    return 0;
}

extern "C" int main(int argc, char *argv[])
{
    psig::signal_manager mgr;
    psig::sigset signals;
    signals += SIGINT;
    signals += SIGTERM;
    signals += SIGHUP;

    mgr.block_signals(signals, std::chrono::nanoseconds(1000000000L));
    return mgr.exec(handle_signal, handle_exit);
}
