# psig ![Build Status](https://travis-ci.org/cpppractitioners/psig.svg?branch=master)
Synchronous Handling of Asynchronous POSIX Signals

#### Description
psig is an interface for handling POSIX signals without the restrictions of signal handlers.  It is designed for use on Linux and is header-only.

#### Requirements
psig requires a fairly recent version GNU autotools and a recent version of GNU autoconf-archive to build and install.  All the code used simply requires a compliant C++11 compiler.

#### Installation
psig is built using GNU autotools.

After getting the source, from the root of the source directory, run the following

```
./bootstrap.sh
./configure
make
make install
```

#### Examples
##### Direct
```c++
#include <psig/psig.hpp>
#include <iostream>

extern "C"
int main(int argc, char *argv[])
{
    App app;
    app.parse(argc, argv);

    psig::this_thread::fill_mask(); // block all signals

    app.start(); // spawn threads

    psig::sigset signals{ SIGTERM, SIGINT, SIGHUP };
    psig::signum_t signum = psig::wait(signals);

    while (true)
    {
        switch (signum)
    	{
            case SIGTERM:
            case SIGINT:
                app.stop();
                return 0;
            case SIGHUP:
                app.reload();
                break;
            default:
                break;
        }
    }
}
```

##### Signal Manager
###### Synchronous
```c++
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
```

###### Asynchronous
```c++
#include <psig/psig.hpp>
#include <iostream>
#include <atomic>

std::atomic< bool > g_running(true);

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
    g_running = false;
    
    return 0;
}

extern "C" int main(int argc, char *argv[])
{
    psig::sigset signals{SIGINT, SIGTERM, SIGHUP};
    psig::signal_manager::block_signals(signals);

    psig::signal_manager::exec_async(handle_signal, handle_exit);
    
    // do initialization here
    
    while (g_running)
    {
        // application's event loop
    }
    
    psig::signal_manager::wait_for_exec_async();
    return psig::signal_manager::exit_code();
}
```

#### Authors
Chris Knight, Daniel C. Dillon
