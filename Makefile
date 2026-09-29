CXX      := g++
QTINC    := /usr/include/x86_64-linux-gnu/qt6
CXXFLAGS := -std=c++17 -Wall -Wextra -g -fPIC -MMD -MP
CXXFLAGS += -I$(QTINC) -I$(QTINC)/QtCore -I$(QTINC)/QtGui -I$(QTINC)/QtWidgets
LDLIBS   := -lQt6Widgets -lQt6Gui -lQt6Core

# ---- EDIT THESE ----
SRC := main.cpp
SRC += Wires/Wire.cpp
SRC += UI/MainSimulationWindow.cpp

INCDIRS := .
# INCDIRS += include
# INCDIRS += src/core
# --------------------

CXXFLAGS += $(addprefix -I,$(INCDIRS))

OBJ := $(patsubst %.cpp,build/%.o,$(SRC))
DEP := $(OBJ:.o=.d)
BIN := build/myqt

all: $(BIN)

$(BIN): $(OBJ)
	@mkdir -p $(@D)
	$(CXX) $(OBJ) -o $@ $(LDLIBS)

build/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	./$(BIN)

clean:
	rm -rf build

-include $(DEP)

.PHONY: all run clean