#ifndef RANDOM_UTILS_H
#define RANDOM_UTILS_H

#include <cstdint>
#include <random>
#include <stdexcept>

#ifndef _MSC_VER
#define NOEXCEPT noexcept
#else
#define NOEXCEPT
#endif

#if defined (_MSC_VER)
#define thread_local __declspec( thread )
#elif defined (__GCC__)
#define thread_local __thread
#endif

// N.B. largest uint32_t prime, but could be chosen dynamically if needed
static const uint32_t HASH_PRIME = 4294967291U;

// return a random size_t >= min and <= max if provided
inline unsigned int random_uint(unsigned int min, unsigned int max = std::numeric_limits<unsigned int>::max())
{
	// TODO thread-safe?
	static std::random_device random_device;
	static std::mt19937 generator(random_device());

	std::uniform_int_distribution<unsigned int> dist(min, max);
	return dist(generator);
}

// return a random hash function onto [0, range)
template <class K>
std::function<size_t(K)> random_hash(uint32_t range)
{
	if (HASH_PRIME < range) throw std::out_of_range("random_hash requested range is larger than HASH_PRIME");

	uint32_t a = random_uint(1, HASH_PRIME - 1);
	uint32_t b = random_uint(0, HASH_PRIME - 1);

	return [range, a, b](K key) { return ((a * key + b) % HASH_PRIME) % range; };
}

#endif
