#ifndef RANDOM_UTILS_H
#define RANDOM_UTILS_H

#include <cstdint>
#include <random>
#include <stdexcept>

// TODO this hard-coded prime causes infinite loops if too large, why?
// something to do with modular arithmetic and (key % range) == 1 ?
static const uint32_t HASH_PRIME = 2695268753U;

// return a random size_t >= min and <= max if provided
inline unsigned int random_uint(unsigned int min, unsigned int max = std::numeric_limits<unsigned int>::max())
{
#ifdef __GNUG__ // if possible make these RNGs properly thread-local
	static thread_local std::random_device random_device;
	static thread_local std::mt19937 generator(random_device());
#else // just hope it works out without it
	static std::random_device random_device;
	static std::mt19937 generator(random_device());
#endif

	std::uniform_int_distribution<unsigned int> dist(min, max);
	return dist(generator);
}

// return a random hash function onto [0, range)
template <class K>
std::function<size_t(K)> random_hash(size_t range)
{
	if (HASH_PRIME < range) throw std::out_of_range("random_hash requested range is larger than HASH_PRIME");

	uint32_t a = random_uint(1, HASH_PRIME - 1);
	uint32_t b = random_uint(0, HASH_PRIME - 1);

	return [range, a, b](K key) { return ((a * key + b) % HASH_PRIME) % range; };
}

#endif
