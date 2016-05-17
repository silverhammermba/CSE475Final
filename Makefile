CXXFLAGS=-std=c++14 -O2 -Wall -Wextra -Wfatal-errors -Wconversion -Igoogletest/googletest/include
LDFLAGS=-Lgoogletest/googlemock/gtest
LDLIBS=-lpthread -lgtest -lboost_system -lboost_program_options -lboost_thread
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
