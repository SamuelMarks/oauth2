// Reference: https://stackoverflow.com/questions/440133/how-do-i-create-a-random-alpha-numeric-string-in-c

#include <iostream>
#include <ctime>

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

std::string generate_random_string(unsigned int required_length) {
    if ( required_length == 0 ) {
        throw std::runtime_error("random string cannot be the empty string, zero length passed to generator.");
    }
    // set the seed of the random number generator
    // to something random.  If you wanted a repeatable
    // result then you would make this a number you knew.
    srand( (unsigned) time(NULL) * getpid());
    // Find the size of our static array
    size_t N = sizeof(alphanum);
    // declare and reserve some memory for a string of our given length
    std::string tmp_s;
    tmp_s.reserve(required_length);
    for (auto ii = 0; ii < required_length; ++ii) {
        tmp_s += alphanum[rand() % (N - 1)];
    }
    return tmp_s;
}
