#include <psig/psig.hpp>
#include <iostream>

extern "C" int main(int argc, char *argv[])
{
    namespace kps = psig;
    kps::this_thread::fill_mask();

    // Spawn threads here

    kps::sigset signals{SIGTERM, SIGINT, SIGHUP};
    signals += SIGRTMIN;
    signals += SIGRTMIN + 1;
    signals += SIGRTMAX;

    ::siginfo_t info;
    const kps::signum_t signum = kps::wait(signals, &info);

    std::cout << "Received signal: " << signum << " from pid: " << info.si_pid
              << " uid: " << info.si_uid << std::endl;

    kps::this_thread::clear_mask();
    return 0;
}
