
#include <iostream>
#include "config_t.h"

// Print the values of the seed, iters, and name fields as comma separated
// values, with a header row
void config_t::dump() {
    std::cout << "# name, key_max, iters, threads, map, time" << std::endl;
    std::cout << name << ", "
              << key_max << ", "
              << iters << ", "
              << threads << ", "
              << map << ", ";
}
