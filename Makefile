CXXFLAGS=-std=c++11 -Wall -Wextra -Wfatal-errors -ggdb
SRC=$(wildcard *.cpp)
OBJ=$(subst .cpp,.o,$(SRC))
BIN=main

.PHONY: clean

$(BIN): $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $+ $(LDLIBS)

main.o: fastmap.h

clean:
	rm -f $(OBJ) $(BIN)
