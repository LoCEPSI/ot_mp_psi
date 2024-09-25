# Compiler and Flags
CXX        := g++
CXX_FLAGS  := -std=c++20 -O3 -Wall

# Directories
BIN        := bin
SRC        := src
THIRD_PARTY := third_party
INCLUDE    := include
LIB        := lib
BENCHMARK  := tools/benchmark
GENPRIME   := tools/gen_prime
CONFIG     := config

# Libraries
LIBRARIES  := -lntl -lgmp -lm -lpthread

# Executables
EXECUTABLE1 := main
EXECUTABLE2 := benchmark
EXECUTABLE3 := gen_prime

# Detect Operating System
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    INCLUDE += /opt/homebrew/Cellar/ntl/11.5.1/include /opt/homebrew/Cellar/gmp/6.3.0/include /opt/homebrew/Cellar/boost/1.84.0_1/include
    LIB     += /opt/homebrew/Cellar/ntl/11.5.1/lib /opt/homebrew/Cellar/gmp/6.3.0/lib /opt/homebrew/Cellar/boost/1.84.0_1/lib
endif

UNAME := $(shell uname)
ifeq ($(UNAME),Darwin)
    LIBRARIES += -lboost_thread-mt
else
    LIBRARIES += -lboost_thread
endif

# Default Target
all: $(BIN) $(CONFIG) $(BIN)/$(EXECUTABLE1) $(BIN)/$(EXECUTABLE2) $(BIN)/$(EXECUTABLE3)

# Run Target (Fixed to specify which executable to run)
run: all
	@echo "Executing $(EXECUTABLE1)..."
	./$(BIN)/$(EXECUTABLE1)

# Rule to Build Executable1
$(BIN)/$(EXECUTABLE1): $(wildcard $(SRC)/*.cpp) $(wildcard $(SRC)/*/*.cpp) $(wildcard $(THIRD_PARTY)/*/*.cpp) | $(BIN)
	@echo "Building $(EXECUTABLE1)..."
	$(CXX) $(CXX_FLAGS) $(addprefix -I,$(INCLUDE)) $(addprefix -L,$(LIB)) $^ -o $@ $(LIBRARIES)

# Rule to Build Executable2
$(BIN)/$(EXECUTABLE2): $(wildcard $(BENCHMARK)/*.cpp) $(wildcard $(SRC)/*/*.cpp) $(wildcard $(THIRD_PARTY)/*/*.cpp) | $(BIN)
	@echo "Building $(EXECUTABLE2)..."
	$(CXX) $(CXX_FLAGS) $(addprefix -I,$(INCLUDE)) $(addprefix -L,$(LIB)) $^ -o $@ $(LIBRARIES)

# Rule to Build Executable3
$(BIN)/$(EXECUTABLE3): $(wildcard $(GENPRIME)/*.cpp) | $(BIN)
	@echo "Building $(EXECUTABLE3)..."
	$(CXX) $(CXX_FLAGS) $(addprefix -I,$(INCLUDE)) $(addprefix -L,$(LIB)) $^ -o $@ $(LIBRARIES)


$(BIN):
	@echo "Creating directory: $(BIN)"
	mkdir -p $(BIN)


$(CONFIG):
	@echo "Creating directory: $(CONFIG)"
	mkdir -p $(CONFIG)

# Clean Target
clean:
	@echo "Clearing build artifacts..."
	-rm -f $(BIN)/*
	-rm -f $(CONFIG)/*

# Phony Targets
.PHONY: all run clean