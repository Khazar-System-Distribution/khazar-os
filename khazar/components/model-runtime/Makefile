CC      := gcc
SDKDIR := ../../sdk
CFLAGS  := -Wall -Wextra -std=c11 -O2 -pthread -Wno-format-truncation -I$(SDKDIR)/include -I$(SDKDIR)
LDFLAGS := -pthread -L$(SDKDIR) -lai-sdk -lm
TARGET  := ai-model-runtime

INCDIR  := include

MODULES := config inference protocol
SRCS    := main.c $(foreach m,$(MODULES),$m/$m.c)
OBJS    := $(SRCS:.c=.o)

.PHONY: all clean test install

all: $(TARGET)
	@echo "=== Build complete ==="

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "  LD  $@"

%.o: %.c
	$(CC) $(CFLAGS) -I. -I$(INCDIR) -c -o $@ $<
	@echo "  CC  $<"

clean:
	rm -f $(OBJS) $(TARGET)
	rm -f tests/*.o tests/test_runtime
	@echo "  clean done"

test: $(TARGET)
	@echo "--- Unit Tests ---"
	@gcc $(CFLAGS) -I. -I$(INCDIR) tests/test_runtime.c config/config.o inference/inference.o protocol/protocol.o -o tests/test_runtime $(LDFLAGS)
	@./tests/test_runtime
	@rm -f tests/test_runtime

install: $(TARGET)
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(TARGET) $(DESTDIR)/usr/local/bin/$(TARGET)
	install -d $(DESTDIR)/etc
	install -m 644 config.toml.example $(DESTDIR)/etc/ai-model-runtime.toml
	@echo "  install complete"
