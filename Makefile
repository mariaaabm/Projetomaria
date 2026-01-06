CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
PKG_CONFIG ?= pkg-config

GLFW_CFLAGS := $(shell $(PKG_CONFIG) --cflags glfw3 2>/dev/null)
GLFW_LIBS := $(shell $(PKG_CONFIG) --libs glfw3 2>/dev/null)
GLEW_CFLAGS := $(shell $(PKG_CONFIG) --cflags glew 2>/dev/null)
GLEW_LIBS := $(shell $(PKG_CONFIG) --libs glew 2>/dev/null)

INCLUDES := -I./common -I./src
LIBS := $(GLFW_LIBS) $(GLEW_LIBS) -lGL -ldl -pthread

SRC := src/main.cpp src/assets/model.cpp src/game/game_state.cpp src/game/police.cpp src/game/collision.cpp src/game/road.cpp src/menu/menu.cpp
BIN := pista_viewer

all: $(BIN)

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(GLFW_CFLAGS) $(GLEW_CFLAGS) $^ -o $@ $(LIBS)

clean:
	rm -f $(BIN)
