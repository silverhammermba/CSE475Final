CXXFLAGS=-std=c++14 -Wall -Wextra -Wfatal-errors -ggdb -Igoogletest/googletest/include
LDFLAGS=-Lgoogletest/googlemock/gtest
LDLIBS=-lpthread -lgtest
SRC=$(wildcard *.cpp)
OBJ=$(subst .cpp,.o,$(SRC))
BIN=main

.PHONY: clean

$(BIN): $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $+ $(LDLIBS)

main.o: fast_map.h perfect_table.h random_utils.h gtest_include.h test_fast_map.h

clean:
	$(RM) $(OBJ) $(BIN)
