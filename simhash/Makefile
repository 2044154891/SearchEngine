CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wno-deprecated-declarations
INCLUDES := -I.. -I../include

# Binaries
DEDUPE_BIN := dedupe_main

# Sources
DEDUPE_SRCS := dedupe_main.cpp Deduplication.cc

.PHONY: all clean run demo

all: $(DEDUPE_BIN)

$(DEDUPE_BIN): $(DEDUPE_SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

run: $(DEDUPE_BIN)
	./$(DEDUPE_BIN)

clean:
	rm -f $(DEDUPE_BIN)


