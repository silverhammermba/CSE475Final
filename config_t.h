
#include <string>

// store all of our command-line configuration parameters
//
// NB: it's a struct, not a class... do you know the difference?
struct config_t {

  // The maximum key value
    int key_max;

    // The number of iterations for which a test should run
    int iters;

    // A string that is output with all the other information
    std::string name;

    // The number of threads to use
    int threads;

    // Which map?
    std::string map;

    // simple constructor
    config_t()
		: key_max(2560), iters(10000), name("no_name"), threads(1),
          map("multithreaded_rev1")
    { }

    // Print the values of the seed, iters, and name fields
    void dump();
};
