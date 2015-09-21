#include <psig/psig.hpp>
#include <iostream>

#define KTL_CHECK(cond)                            \
    if (!(cond))                                   \
    {                                              \
        std::cout << "Error: ##cond" << std::endl; \
        exit(1);                                   \
    }

void test_mask()
{
    namespace kps = psig;

    kps::sigset newsigset{SIGTERM, SIGINT};
    kps::sigset oldsigset = kps::this_thread::set_mask(newsigset);
    kps::sigset cursigset = kps::this_thread::get_mask();

    std::cout << "Signals in new mask: ";
    for (kps::signum_t signum : newsigset.signals()) std::cout << signum << " ";
    std::cout << std::endl;

    KTL_CHECK(oldsigset.empty());
    KTL_CHECK(newsigset == cursigset);
    KTL_CHECK(oldsigset != cursigset);
    KTL_CHECK(newsigset.has(SIGTERM));
    KTL_CHECK(newsigset.has(SIGINT));
    KTL_CHECK(newsigset.has(SIGHUP) == false);

    newsigset.clear();
    KTL_CHECK(newsigset.empty());
    KTL_CHECK(newsigset.has(SIGTERM) == false);
    KTL_CHECK(newsigset.has(SIGINT) == false);
    KTL_CHECK(newsigset.has(SIGHUP) == false);

    kps::this_thread::clear_mask();
    cursigset = kps::this_thread::get_mask();
    KTL_CHECK(cursigset.empty());
    KTL_CHECK(cursigset.has(SIGTERM) == false);
    KTL_CHECK(cursigset.has(SIGINT) == false);
    KTL_CHECK(cursigset.has(SIGHUP) == false);

    kps::this_thread::fill_mask();
    cursigset = kps::this_thread::get_mask();
    KTL_CHECK(cursigset.empty() == false);
    KTL_CHECK(cursigset.has(SIGTERM));
    KTL_CHECK(cursigset.has(SIGINT));
    KTL_CHECK(cursigset.has(SIGHUP));
    KTL_CHECK(cursigset.has(SIGRTMIN));
    KTL_CHECK(cursigset.has(SIGRTMAX));
    KTL_CHECK(cursigset.has(kps::rt::signum(0)));  // SIGRTMIN
    KTL_CHECK(cursigset.has(kps::rt::signum(1)));  // SIGRTMIN+1
    KTL_CHECK(cursigset.has(kps::rt::signum(kps::rt::sigcount())));  // SIGRTMAX
}

void test_wait()
{
    namespace kps = psig;

    kps::sigset signals{SIGTERM, SIGINT, SIGHUP};
    for (kps::sigcnt_t rtsigcnt = 0; rtsigcnt <= kps::rt::sigcount();
         ++rtsigcnt)
        signals += kps::rt::signum(rtsigcnt);

    std::cout << "Please type Ctrl^C" << std::endl;
    const kps::signum_t signum = kps::wait(signals);
    std::cout << "Received signal: " << signum << std::endl;
}

void test_rt_wait()
{
    namespace kps = psig;

    std::cout << "Please type Ctrl^C" << std::endl;
    const kps::signum_t signum = kps::wait({SIGTERM, SIGINT, SIGHUP});
    std::cout << "Received signal: " << signum << std::endl;
}

void test_timed_wait()
{
    namespace kps = psig;

    std::cout << "Please type Ctrl^C within 2.25 seconds" << std::endl;
    const kps::signum_t signum = kps::wait(
        {SIGTERM, SIGINT, SIGHUP}, std::chrono::nanoseconds(2250000000));
    std::cout << "Received signal: " << signum << std::endl;
}

extern "C" int main(int argc, char *argv[])
{
    test_mask();
    test_wait();
    test_rt_wait();
    test_timed_wait();
    return 0;
}
