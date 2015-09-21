#pragma once

#include <cstring>
#include <chrono>
#include <unordered_set>
#include <vector>
#include <signal.h>

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

}  // namespace psig
