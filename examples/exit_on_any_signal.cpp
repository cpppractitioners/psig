#include <psig/psig.hpp>
#include <iostream>

bool handle_signal(int sig)
{
    std::cout << "Handling signal " << sig << std::endl;

    switch (sig)
    {
        case SIGTERM:
        case SIGHUP:
        case SIGINT:
            return false;
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
    psig::sigset signals{SIGINT, SIGTERM, SIGHUP};
    psig::signal_manager::block_signals(signals);

    // do application initialization here

    return psig::signal_manager::exec(handle_signal, handle_exit);
}
