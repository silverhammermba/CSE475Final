#pragma once
#ifdef TESTING // don't do anything if we aren't testing
#ifdef _WIN32 // supress warnings on Windows
#pragma warning (push)
#pragma warning(disable: 4251 4275)
#include <gmock\gmock.h>
#pragma warning( pop)
#else
#include <gtest/gtest.h>
#endif
#endif
