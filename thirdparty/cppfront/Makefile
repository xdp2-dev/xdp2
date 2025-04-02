# Set USE_HOST_TOOLS to use HOST_CC for building compiler
USE_HOST_TOOLS := y
include ../../src/config.mk

TARGET = cppfront-compiler
STD_FLAGS = -std=c++20
SRC = source/cppfront.cpp

all: compiler

compiler:
	$(HOST_CXX) $(STD_FLAGS) $(SRC) -o $(TARGET)
