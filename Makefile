CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pedantic
INCLUDES := -Iinclude

TARGET := wikiTraverser
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
SRC_DIR := src

SRCS := \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/community_detection.cpp \
	$(SRC_DIR)/digraph.cpp \
	$(SRC_DIR)/digraph_builder.cpp \
	$(SRC_DIR)/graph_algorithms.cpp \
	$(SRC_DIR)/title_utils.cpp \
	$(SRC_DIR)/visualization_exporter.cpp

OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
