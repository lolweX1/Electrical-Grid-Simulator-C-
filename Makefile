CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -fPIC
CXXFLAGS += $(shell pkg-config --cflags Qt6Widgets)
LDLIBS   := $(shell pkg-config --libs Qt6Widgets)

SRC := $(wildcard src/*.cpp)
OBJ := $(SRC:src/%.cpp=build/%.o)
BIN := build/myqt

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDLIBS)

build/%.o: src/%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	./$(BIN)

clean:
	rm -rf build

.PHONY: all run clean