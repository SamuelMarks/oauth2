#include <random>
#include "random_string.h"

std::string generate_random_string(const size_t required_length) {
    constexpr char alphanum[] = "0123456789"
                                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "abcdefghijklmnopqrstuvwxyz";
    std::uniform_int_distribution<int> d(0, 15);

    std::random_device rd1;
    std::string rand_str (required_length, '\0');
    for (auto& c : rand_str) {
        c = alphanum[d(rd1)];
    }

    return rand_str;
}
