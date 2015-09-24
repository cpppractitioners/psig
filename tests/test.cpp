#include <psig/psig.hpp>
#include <iostream>

#define KTL_CHECK(cond)                            \
    if (!(cond))                                   \
    {                                              \
        std::cout << "Error: " #cond << std::endl; \
        exit(1);                                   \
    }

void test_mask()
{
    psig::sigset emptysigset = psig::this_thread::get_mask();
    KTL_CHECK(emptysigset.empty());

    psig::sigset newsigset{SIGTERM, SIGINT};
    psig::sigset oldsigset = psig::this_thread::set_mask(newsigset);
    psig::sigset cursigset = psig::this_thread::get_mask();

    std::cout << "Signals in new mask: ";
    for (psig::signum_t signum : newsigset.signals()) std::cout << signum << " ";
    std::cout << std::endl;

    std::cout << "Signals in old mask: ";
    for (psig::signum_t signum : oldsigset.signals()) std::cout << signum << " ";
    std::cout << std::endl;

    std::cout << "Signals in cur mask: ";
    for (psig::signum_t signum : cursigset.signals()) std::cout << signum << " ";
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

    psig::this_thread::clear_mask();
    cursigset = psig::this_thread::get_mask();
    KTL_CHECK(cursigset.empty());
    KTL_CHECK(cursigset.has(SIGTERM) == false);
    KTL_CHECK(cursigset.has(SIGINT) == false);
    KTL_CHECK(cursigset.has(SIGHUP) == false);

    psig::this_thread::fill_mask();
    cursigset = psig::this_thread::get_mask();
    KTL_CHECK(cursigset.empty() == false);
    KTL_CHECK(cursigset.has(SIGTERM));
    KTL_CHECK(cursigset.has(SIGINT));
    KTL_CHECK(cursigset.has(SIGHUP));
    KTL_CHECK(cursigset.has(SIGRTMIN));
    KTL_CHECK(cursigset.has(SIGRTMAX));
    KTL_CHECK(cursigset.has(psig::rt::signum(0)));  // SIGRTMIN
    KTL_CHECK(cursigset.has(psig::rt::signum(1)));  // SIGRTMIN+1
    KTL_CHECK(cursigset.has(psig::rt::signum(psig::rt::sigcount())));  // SIGRTMAX
}

void test_wait()
{
    psig::sigset signals{SIGTERM, SIGINT, SIGHUP};
    for (psig::sigcnt_t rtsigcnt = 0; rtsigcnt <= psig::rt::sigcount();
         ++rtsigcnt)
        signals += psig::rt::signum(rtsigcnt);

    std::cout << "Please type Ctrl^C" << std::endl;
    const psig::signum_t signum = psig::wait(signals);
    std::cout << "Received signal: " << signum << std::endl;
}

void test_rt_wait()
{
    std::cout << "Please type Ctrl^C" << std::endl;
    const psig::signum_t signum = psig::wait({SIGTERM, SIGINT, SIGHUP});
    std::cout << "Received signal: " << signum << std::endl;
}

void test_timed_wait()
{
    std::cout << "Please type Ctrl^C within 2.25 seconds" << std::endl;
    const psig::signum_t signum = psig::wait(
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
