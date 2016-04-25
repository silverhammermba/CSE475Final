CXXFLAGS=-std=c++14 -Wall -Wextra -Wfatal-errors -Wconversion -ggdb -Igoogletest/googletest/include
LDFLAGS=-Lgoogletest/googlemock/gtest
LDLIBS=-lpthread -lgtest -lboost_program_options
SRC=$(wildcard *.cpp)
OBJ=$(SRC:.cpp=.o)
BIN=main

.PHONY: clean

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $+ $(LDLIBS)

clean:
	$(RM) $(OBJ) $(BIN)

# automatic dependency generation

.depend: $(SRC) $(wildcard *.h)
	$(CXX) $(CXXFLAGS) -MM $^ > .depend

include .depend
