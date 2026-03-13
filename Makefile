# Matryoshka — C++ build (Ubuntu, clang)
CXX      ?= clang++
CXXFLAGS  = -std=c++17 -Wall -Wextra -O2 -g -Isrc
LDFLAGS   =

DEVICE_SRC = src/main.cpp src/fw_parser.cpp src/protocol.cpp
DEVICE_OBJ = $(DEVICE_SRC:.cpp=.o)
TARGET    = matryoshka

UPDATER_SRC = src/updater.cpp
UPDATER_OBJ = $(UPDATER_SRC:.cpp=.o)
UPDATER     = updater

.PHONY: all clean seed

all: $(TARGET) $(UPDATER)

$(TARGET): $(DEVICE_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(UPDATER): $(UPDATER_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

seed: tools/mkseed_fw
	@mkdir -p corpus
	@cd corpus && ../tools/mkseed_fw 2>/dev/null || (cd .. && ./tools/mkseed_fw && mv valid_fw.bin corpus/)

tools/mkseed_fw: tools/mkseed_fw.cpp
	$(CXX) $(CXXFLAGS) -Isrc -o $@ $<

clean:
	rm -f $(TARGET) $(UPDATER) $(DEVICE_OBJ) $(UPDATER_OBJ) tools/mkseed_fw corpus/valid_fw.bin
