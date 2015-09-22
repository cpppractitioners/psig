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
 
#pragma once

#include <cstring>
#include <thread>
#include <chrono>
#include <unordered_set>
#include <vector>
#include <signal.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <functional>

extern "C" {
inline void psig_signal_handler(int signum) {}
}

namespace psig
{
class sigset;
namespace this_thread
{
namespace impl
{
sigset get_mask();
}  // namespace impl
}  // namespace this_thread

typedef int signum_t;
typedef int sigcnt_t;

class sigset
{
   public:
    sigset() noexcept { clear(); }
    sigset(const sigset &s) noexcept : m_signals(s.m_signals)
    {
        copy_handle(s);
    }
    sigset(std::initializer_list< signum_t > il) noexcept { init_handle(il); }
    ~sigset() noexcept = default;

    explicit sigset(const signum_t signum) noexcept { init_handle({signum}); }
    explicit sigset(const bool) noexcept { fill(); }

    sigset &operator=(const sigset &that) noexcept
    {
        copy_handle(that);
        m_signals = that.m_signals;
        return *this;
    }

    typedef ::sigset_t *native_handle_type;
    native_handle_type native_handle() const
    {
        return const_cast< native_handle_type >(&m_handle);
    }

    typedef std::unordered_set< signum_t > signals_type;
    const signals_type &signals() const { return m_signals; }

    void insert(const signum_t signum) noexcept
    {
        ::sigaddset(&m_handle, signum);
        m_signals.insert(signum);
    }
    void erase(const signum_t signum) noexcept
    {
        ::sigdelset(&m_handle, signum);
        m_signals.erase(signum);
    }

    sigset &operator+=(const signum_t signum) noexcept
    {
        insert(signum);
        return *this;
    }
    sigset &operator-=(const signum_t signum) noexcept
    {
        erase(signum);
        return *this;
    }

    bool operator==(const sigset &that) const
    {
        return (m_signals == that.m_signals);
    }
    bool operator!=(const sigset &that) const
    {
        return (m_signals != that.m_signals);
    }

    bool has(const signum_t signum)
    {
        return (m_signals.find(signum) != m_signals.end());
    }
    bool empty() const { return (m_signals.size() == 0); }

    void fill() { ::sigfillset(&m_handle); }
    void clear()
    {
        ::sigemptyset(&m_handle);
        m_signals.clear();
    }

   private:
    friend sigset this_thread::impl::get_mask();  // for init_handle()

    void copy_handle(const sigset &that)
    {
        std::memcpy(&m_handle, &that.m_handle, sizeof(m_handle));
    }
    void init_handle(std::initializer_list< signum_t > signums)
    {
        clear();
        for (signum_t signum : signums)
        {
            insert(signum);
        }
    }
    void init_handle()
    {
        for (signum_t signum : std::vector< signum_t >{
                 SIGTERM, SIGINT, SIGHUP, SIGUSR1, SIGUSR2, SIGPWR, SIGALRM})
            if (::sigismember(&m_handle, signum))
                m_signals.insert(signum);

        for (signum_t signum = 0; signum <= SIGRTMAX - SIGRTMIN; ++signum)
            if (::sigismember(&m_handle, SIGRTMIN + signum))
                m_signals.insert(SIGRTMIN + signum);
    }

   private:
    ::sigset_t m_handle;
    signals_type m_signals;
};

namespace rt
{
inline signum_t sigmin() { return SIGRTMIN; }
inline signum_t sigmax() { return SIGRTMAX; }
inline signum_t sigcount() { return sigmax() - sigmin(); }
inline signum_t signum(const sigcnt_t rtsigcnt) { return sigmin() + rtsigcnt; }
inline sigcnt_t sigcnt(const signum_t rtsignum) { return rtsignum - sigmin(); }
}  // namespace rt

namespace this_thread
{
namespace impl
{
inline sigset set_mask(const int how, const sigset &newset)
{
    sigset oldset;
    ::pthread_sigmask(how, newset.native_handle(), oldset.native_handle());
    return oldset;
}

inline sigset get_mask()
{
    sigset oldset;
    ::pthread_sigmask(SIG_UNBLOCK, nullptr, oldset.native_handle());
    oldset.init_handle();
    return oldset;
}
}  // namespace impl

inline sigset get_mask() noexcept { return impl::get_mask(); }
inline sigset set_mask(const sigset &newset) noexcept
{
    return impl::set_mask(SIG_SETMASK, newset);
}

inline sigset fill_mask() noexcept
{
    return impl::set_mask(SIG_SETMASK, sigset(true));
}
inline sigset clear_mask() noexcept
{
    return impl::set_mask(SIG_SETMASK, sigset());
}

inline sigset add_mask(const sigset &addset) noexcept
{
    return impl::set_mask(SIG_BLOCK, addset);
}
inline sigset sub_mask(const sigset &subset) noexcept
{
    return impl::set_mask(SIG_UNBLOCK, subset);
}

inline sigset add_mask(const signum_t signum) noexcept
{
    return add_mask(sigset(signum));
}
inline sigset sub_mask(const signum_t signum) noexcept
{
    return sub_mask(sigset(signum));
}
}  // namespace this_thread

namespace this_process
{
inline void set_action(const sigset &signals)
{
    struct ::sigaction action;
    ::sigfillset(&action.sa_mask);
    action.sa_sigaction = nullptr;
    action.sa_restorer = nullptr;
    action.sa_handler = &::psig_signal_handler;
    action.sa_flags = 0;

    for (signum_t signal : signals.signals())
        ::sigaction(signal, &action, nullptr);
}
}  // namespace this_process

inline signum_t wait(const sigset &signals, ::siginfo_t *info = nullptr)
{
    this_process::set_action(signals);
    const sigset oldset = this_thread::set_mask(signals);
    const signum_t signum = ::sigwaitinfo(signals.native_handle(), info);
    this_thread::set_mask(oldset);
    return signum;
}

inline signum_t wait(const sigset &signals,
                     std::chrono::nanoseconds timeout,
                     ::siginfo_t *info = nullptr)
{
    ::timespec ts;
    ts.tv_sec =
        std::chrono::duration_cast< std::chrono::seconds >(timeout).count();
    if (ts.tv_sec)
        ts.tv_nsec = timeout.count() - (ts.tv_sec * std::nano().den);
    else
        ts.tv_nsec = timeout.count();

    this_process::set_action(signals);
    const sigset oldset = this_thread::set_mask(signals);
    const signum_t signum = ::sigtimedwait(signals.native_handle(), info, &ts);
    this_thread::set_mask(oldset);
    return signum;
}

class signal_manager
{
   public:
    static inline bool block_signals(
        const std::chrono::nanoseconds &timeout_nsec =
            std::chrono::nanoseconds(0))
    {
        return instance().block_signals_internal(timeout_nsec);
    }

    static inline bool block_signals(
        const sigset &signals,
        const std::chrono::nanoseconds &timeout_nsec =
            std::chrono::nanoseconds(0))
    {
        return instance().block_signals_internal(signals, timeout_nsec);
    }

    static inline int exec(const std::function< bool(int)> &signalHandler,
                           const std::function< int() > &exitHandler)
    {
        return instance().exec_internal(signalHandler, exitHandler);
    }

    static inline int exec(const std::function< bool(int)> &signalHandler)
    {
        return exec(signalHandler, &signal_manager::default_exit_handler);
    }

    static inline int exec(const std::function< int() > &exitHandler)
    {
        return exec(&signal_manager::default_signal_handler, exitHandler);
    }

    static inline int exec()
    {
        return exec(&signal_manager::default_signal_handler,
                    &signal_manager::default_exit_handler);
    }
    
    static inline void exec_async(const std::function< bool(int)> &signalHandler,
                           const std::function< int() > &exitHandler)
    {
        instance().exec_async_internal(signalHandler, exitHandler);
    }

    static inline void exec_async(const std::function< bool(int)> &signalHandler)
    {
        exec_async(signalHandler, &signal_manager::default_exit_handler);
    }

    static inline void exec_async(const std::function< int() > &exitHandler)
    {
        exec_async(&signal_manager::default_signal_handler, exitHandler);
    }

    static inline void exec_async()
    {
        exec_async(&signal_manager::default_signal_handler,
                    &signal_manager::default_exit_handler);
    }
    
    static inline void wait_for_exec_async()
    {
        instance().wait_for_exec_async_internal();
    }

    static inline void kill(int sig = SIGINT) { ::kill(0, sig); }

    static inline void stop(int sig = SIGINT) { instance().stop_internal(sig); }
    
    static inline int exit_code() { return instance().exit_code_internal(); }

   private:
    static inline signal_manager &instance()
    {
        static signal_manager mgr;
        return mgr;
    }

    inline signal_manager() : m_running(false), m_exit_code(0) {}
    signal_manager(const signal_manager &rhs) = delete;
    signal_manager &operator=(const signal_manager &rhs) = delete;

    inline bool block_signals_internal(
        const std::chrono::nanoseconds &timeout_nsec =
            std::chrono::nanoseconds(0))
    {
        m_timeout_nsec = timeout_nsec;

        this_thread::fill_mask();

        m_signals += SIGHUP;
        m_signals += SIGINT;
        m_signals += SIGTERM;

        m_running = true;
        return true;
    }

    inline bool block_signals_internal(
        const sigset &signals,
        const std::chrono::nanoseconds &timeout_nsec =
            std::chrono::nanoseconds(0))
    {
        m_timeout_nsec = timeout_nsec;

        this_thread::fill_mask();

        m_signals = signals;

        m_running = true;
        return true;
    }

    inline int exec_internal(const std::function< bool(int)> &signalHandler,
                             const std::function< int() > &exitHandler)
    {
        while (m_running)
        {
            ::siginfo_t info;
            signum_t signum;

            if (m_timeout_nsec > std::chrono::nanoseconds(0))
            {
                signum = wait(m_signals, m_timeout_nsec, &info);
            }
            else
            {
                signum = wait(m_signals, &info);
            }

            if (signum > 0)
            {
                if (!signalHandler(signum))
                {
                    m_running = false;
                }
            }
        }

        m_exit_code = exitHandler();
        return m_exit_code;
    }
    
    inline void exec_internal_noret(const std::function< bool(int)> &signalHandler,
                             const std::function< int() > &exitHandler)
    {
        exec_internal(signalHandler, exitHandler);
    }
    
    inline void exec_async_internal(const std::function< bool(int)> &signalHandler,
                             const std::function< int() > &exitHandler)
    {
        m_thread.reset(new std::thread(std::bind(&signal_manager::exec_internal_noret, this, signalHandler, exitHandler)));
    }
    
    inline void wait_for_exec_async_internal()
    {
        if (m_thread)
        {
            m_thread->join();
        }
    }

    inline void stop_internal(int sig = SIGINT)
    {
        m_running = false;
        ::kill(0, sig);
        
        if (m_thread)
        {
            m_thread->join();
        }
    }
    
    inline int exit_code_internal() { return m_exit_code; }

   private:
    static inline int default_exit_handler() { return 0; }

    static inline bool default_signal_handler(int sig) { return false; }

   private:
    sigset m_signals;
    std::atomic< bool > m_running;
    std::chrono::nanoseconds m_timeout_nsec;
    std::unique_ptr< std::thread > m_thread;
    int m_exit_code;
};

}  // namespace psig
