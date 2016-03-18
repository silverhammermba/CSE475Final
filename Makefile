CXXFLAGS=-std=c++14 -Wall -Wextra -Wfatal-errors -ggdb
SRC=$(wildcard *.cpp)
OBJ=$(subst .cpp,.o,$(SRC))
BIN=main

.PHONY: clean

$(BIN): $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $+ $(LDLIBS)

main.o: fastmap.h random_utils.h

clean:
	$(RM) $(OBJ) $(BIN)
