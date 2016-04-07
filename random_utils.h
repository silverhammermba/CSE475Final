#pragma once
#ifndef RANDOM_UTILS_H
#define RANDOM_UTILS_H

#include <random>

// return a random size_t >= min and <= max if provided
inline size_t random_uint(size_t min, size_t max = std::numeric_limits<size_t>::max())
{
	// TODO thread-safe?
	static std::random_device random_device;
	static std::mt19937 generator(random_device());

	std::uniform_int_distribution<size_t> dist(min, max);
	return dist(generator);
}

// test if p is prime
inline bool is_prime(size_t p)
{
	if (p < 2) return false;
	if (p == 2) return true;
	if (p % 2 == 0) return false;

	for (size_t i = 3; (i * i) <= p; i += 2)
	{
		if (p % i == 0) return false;
	}

	return true;
}

// return a random prime >= min
inline size_t random_prime_at_least(size_t min)
{
	while (true)
	{
		size_t p = random_uint(min);
		if (!is_prime(p)) continue;
		return p;
	}
}

// return a random hash function onto [0, range)
template <class K>
std::function<size_t(K)> random_hash(size_t range)
{
	size_t p = random_prime_at_least(range);
	size_t a = random_uint(1, p - 1);
	size_t b = random_uint(0, p - 1);

	return [range, p, a, b](K key) { return ((a * key + b) % p) % range; };
}

#endif
