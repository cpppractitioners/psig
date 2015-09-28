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
#include <atomic>

std::atomic< bool > g_running(true);

bool handle_signal(int sig, const siginfo_t& info)
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

extern "C" int main(int argc, char* argv[])
{
    psig::signal_manager::block_all_signals();

    psig::sigset signals{SIGINT, SIGTERM, SIGHUP};
    psig::signal_manager::exec_async(signals, handle_signal, handle_exit);

    // do initialization here

    while (g_running)
    {
        // application's event loop
    }

    psig::signal_manager::wait_for_exec_async();
    return psig::signal_manager::exit_code();
}
