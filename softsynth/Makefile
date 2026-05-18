CXX := g++
CXXFLAGS := -std=c++11 -O3 -Wall -Wextra -fPIC \
  -Wno-unused-parameter -Wno-enum-compare -Wno-narrowing \
  -Wno-unused-variable -Wno-maybe-uninitialized -fno-strict-aliasing
INCLUDES := -I.
DEFINES := -DDISABLE_WAVETABLE_LFOS

# LV2 plugin flags (set LV2DIR if pkg-config not available)
LV2_CFLAGS := $(shell pkg-config --cflags lv2 2>/dev/null || echo "-I/usr/include/lv2")
LV2_LDFLAGS := $(shell pkg-config --libs lv2 2>/dev/null || echo "")

VOICE_SRCS := voicecard/voice.cc voicecard/oscillator.cc \
              voicecard/resources.cc avrlib/random.cc patches.cc

VOICE_OBJS := $(VOICE_SRCS:.cc=.o)

SOFTSYNTH_SRCS := host/main.cc $(VOICE_SRCS)

TEST_SRCS := tests/test_all.cc $(VOICE_SRCS)

all: ambika_softsynth

# Standalone synth
ambika_softsynth: $(SOFTSYNTH_SRCS:.cc=.o)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Test suite
test: ambika_test
	./ambika_test

test_all: ambika_test
	./ambika_test

ambika_test: tests/test_all.o $(VOICE_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# PluginVoiceManager standalone test (no SDK needed)
test_plugin: tests/test_plugin_manager.o $(VOICE_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@_runner
	./$@_runner

# LV2 host integration test (requires lv2-dev + ambika.so)
test_lv2: ambika.so tests/test_lv2_host.cc
	$(CXX) -std=c++11 -O2 tests/test_lv2_host.cc \
	  $(shell pkg-config --cflags --libs lv2 2>/dev/null || echo "-I/usr/include/lv2") \
	  -I. -ldl -o test_lv2_host
	LD_LIBRARY_PATH=. ./test_lv2_host

# LV2 plugin
ambika.so: host/ambika_lv2.cpp $(VOICE_OBJS)
	$(CXX) $(CXXFLAGS) -fPIC -shared $(LV2_CFLAGS) $(INCLUDES) $(DEFINES) \
	  $^ -o $@

lv2: ambika.so
	@mkdir -p Ambika.lv2
	cp ambika.so Ambika.lv2/
	cp host/ambika_lv2.ttl Ambika.lv2/manifest.ttl
	@echo "LV2 plugin built: Ambika.lv2/"
	@echo "Install: cp -r Ambika.lv2 ~/.lv2/"

.cc.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(DEFINES) -c $< -o $@

test_clean:
	rm -f test_*.wav filt_*.wav

clean:
	rm -f $(SOFTSYNTH_SRCS:.cc=.o) $(VOICE_OBJS) ambika_softsynth ambika_test
	rm -f test_*.wav filt_*.wav tests/test_all.o ambika.so
	rm -f test_plugin_runner test_lv2_host
	rm -rf Ambika.lv2 Ambika.vst3 build_win

distclean: clean
	rm -rf vst3sdk

.PHONY: all clean test test_clean lv2 distclean
