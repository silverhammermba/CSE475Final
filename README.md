# CSE475 Project #

This is an implementation of a [dynamic perfect hash table][dph] (DPH) for C++.
A DPH is a two-tiered hash table that provides extremely fast (constant time)
lookups, inserts, and deletes at the expense of a large (yet linear) memory
footprint. This implementation actually defines two classes that both provide an
interface similar to `std::map`: FastLookupMap and FastMap.

[dph]: https://en.wikipedia.org/wiki/Dynamic_perfect_hashing

## FastLookupMap ##

This class is used internally by the DPH but can also be used separately, if
desired. It provides constant-time lookups and deletes at the expense of a
quadratic memory footprint. It does this by ensuring that the underlying hash
table always remains collision-free, thus inserts will occasionally trigger a
slow, expensive rehash. Excessive rehashing can be avoided if the desired final
size is known and provided at the time of construction.

## FastMap ##

This class implements the actual DPH. It uses an internal hash table of
FastLookupMaps to minimize the number of rehashes following an insert. This
allows for fast inserts on top of the fast lookups and deletes provided by the
FastLookupMaps. It also allocates FastLookupMaps in a way that the overall
memory consumption is linear despite each FastLookupMap's memory need being
quadratic.

In order to keep insert costs consistent, occasional rehashes are required
following inserts and deletes. However these rehashes are triggered at a rate
such that the amortized cost of inserts and deletes remains O(1).

## Building ##

To use these classes in your C++ program, simply include the `fast_map.h` or
`fast_lookup_map.h` headers.

If you want to run the unit tests or speed tests, you must build the project.
You must have [Boost][boost] installed on your system (specifically
the thread and program_options libraries). You will also need to set up Google
Test. Afterwards you can simply `make`.

[boost]: http://www.boost.org

### Google Test ###

The unit tests require [Google Test][gt]. Due to the design of Google Test, it
is usually not provided as a pre-built binary and instead must be built
per-project.

On a Unix-like system, from this project's directory, simply run the following
commands:

    git clone https://github.com/google/googletest.git
    cd googletest
    cmake . && make

[gt]: https://github.com/google/googletest

## Running ##

Run `main`. The default behavior is to run the speed test. Run with `-h` to see
options that can be changed for the speed test. To run unit tests instead, use
`-u`.
