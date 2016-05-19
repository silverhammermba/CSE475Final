CXXFLAGS=-std=c++14 -O2 -Wall -Wextra -Wfatal-errors -Wconversion
LDFLAGS=
LDLIBS=-lpthread -lboost_program_options
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
