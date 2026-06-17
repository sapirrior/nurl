CC      = gcc
VERSION ?= 0.7.0
CFLAGS  = -std=c11 -Wall -Wextra -Os -ffunction-sections -fdata-sections \
          -fno-ident -D_GNU_SOURCE -DNURL_VERSION=\"$(VERSION)\" \
          -Isrc -Isrc/cli -Isrc/cli/parser -Isrc/cli/runner \
          -Isrc/engine -Isrc/engine/net -Isrc/engine/tls -Isrc/engine/http -Isrc/engine/utils -Isrc/compat -Isrc/errors
LDFLAGS = -Wl,-Bstatic -lssl -lcrypto -Wl,-Bdynamic -lpthread -ldl -lz \
          -Wl,--gc-sections -s

SRCS    := $(shell find src -name '*.c')
OBJS    := $(SRCS:src/%.c=build/%.o)
TARGET  ?= nurl

ifeq ($(WINDOWS),1)
  CC       = x86_64-w64-mingw32-gcc
  LDFLAGS += -lws2_32 -lshlwapi
  TARGET   = nurl.exe
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build $(TARGET)

.PHONY: all clean
