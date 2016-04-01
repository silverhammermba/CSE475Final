CXXFLAGS=-std=c++14 -Wall -Wextra -Wfatal-errors -ggdb -Igoogletest/googletest/include
LDFLAGS=-Lgoogletest/googlemock/gtest
LDLIBS=-lpthread -lgtest
SRC=$(wildcard *.cpp)
HEADERS=$(wildcard *.h)
OBJ=$(subst .cpp,.o,$(SRC))
BIN=main

.PHONY: clean

$(BIN): $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $+ $(LDLIBS)

main.o: $(HEADERS)

clean:
	$(RM) $(OBJ) $(BIN)
