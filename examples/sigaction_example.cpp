/* Copyright (c) 2015, Chris Knight, Daniel C. Dillon
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <psig/psig.hpp>
#include <iostream>
#include <unistd.h>

int g_raw1 = 0;
int g_raw2 = 0;

void raw_signal_sigaction(int sig, ::siginfo_t* info, void*)
{
    std::cout << "raw handler: " << sig << std::endl;
    g_raw1 = 1;
}

void raw_signal_handler(int sig)
{
    std::cout << "raw handler: " << sig << std::endl;
    g_raw2 = 2;
}

bool handle_signal(int sig, const siginfo_t info)
{
    std::cout << "Handling signal " << sig << std::endl;

    switch (sig)
    {
        case SIGINT:
        case SIGHUP:
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

extern "C" int main(int argc, char* argv[])
{
    psig::signal_manager::block_all_signals();

    struct ::sigaction action;
    action.sa_sigaction = nullptr;
    action.sa_restorer = nullptr;
    action.sa_handler = &raw_signal_handler;
    action.sa_flags = 0;

    ::sigaction(SIGTERM, &action, nullptr);

    // do application initialization here

    psig::sigset signals{SIGINT, SIGHUP};
    int retval = psig::signal_manager::exec(signals, handle_signal, handle_exit);
    if (g_raw1)
    {
        std::cout << "raw_signal_handler() was called then signal_manager_handler()." << std::endl;
    }

    if (g_raw2)
    {
        std::cout << "signal_manager_handler() was called then raw_signal_handler()." << std::endl;
    }
}
