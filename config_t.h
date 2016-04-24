#include <ostream>
#include <string>

// stores our command-line config parameters
struct config_t
{
	std::string name; // string to identify test run
	int key_max;      // maximum key size
	int threads;      // number of threads to use
	int iters;        // how many operations each thread performs

	// simple constructor
	config_t(const std::string& _name, int _key_max, int _threads, int _iters)
		: name {_name}, key_max {_key_max}, threads {_threads}, iters {_iters}
	{}

	// print the config's values
	friend std::ostream& operator<< (std::ostream& output, const config_t& cfg)
	{
		output << "# name, key_max, iters, threads, time" << std::endl
		       << cfg.name << ", "
		       << cfg.key_max << ", "
		       << cfg.iters << ", "
		       << cfg.threads << ", ";
		return output;
	}
};
