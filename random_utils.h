#ifndef RANDOM_UTILS_H
#define RANDOM_UTILS_H

#include <random>
#include <stdexcept>

#ifndef _MSC_VER
#define NOEXCEPT noexcept
#else
#define NOEXCEPT
#endif

// return a random size_t >= min and <= max if provided
inline unsigned int random_uint(unsigned int min, unsigned int max = std::numeric_limits<unsigned int>::max())
{
	// TODO thread-safe?
	static std::random_device random_device;
	static std::mt19937 generator(random_device());

	std::uniform_int_distribution<unsigned int> dist(min, max);
	return dist(generator);
}

// test if p is prime
inline bool is_prime(unsigned int p)
{
	if (p < 2) return false;
	if (p == 2) return true;
	if (p % 2 == 0) return false;

	for (unsigned int i = 3; (i * i) <= p; i += 2)
	{
		if (p % i == 0) return false;
	}

	return true;
}

// return a random prime >= min
inline unsigned int random_prime_at_least(unsigned int min)
{
	while (true)
	{
		unsigned int p = random_uint(min);
		if (!is_prime(p)) continue;
		return p;
	}
}

// return a random hash function onto [0, range) using prime p. may change p if it is too small
template <class K>
std::function<size_t(K)> random_hash(size_t range, unsigned int& p)
{
	if (range > std::numeric_limits<unsigned int>::max()) throw std::out_of_range("random_hash requested range is too large");
	if (p < range) p = random_prime_at_least((unsigned int)range);

	unsigned int a = random_uint(1, p - 1);
	unsigned int b = random_uint(0, p - 1);

	return [range, p, a, b](K key) { return ((a * key + b) % p) % range; };
}

#endif
