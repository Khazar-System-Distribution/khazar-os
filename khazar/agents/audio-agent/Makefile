CC      := gcc
SDKDIR = ../../sdk
CFLAGS := -Wall -Wextra -std=c11 -O2 -pthread -Wno-format-truncation
INCLUDES = -I. -I../../sdk/include -I../../sdk
LDFLAGS := -pthread -L$(SDKDIR) -lai-sdk
TARGET  := ai-audio-agent

.PHONY: all clean test install

all: $(TARGET)
	@echo "=== Build complete ==="

$(TARGET): main.c
	$(CC) $(INCLUDES) $(CFLAGS) -o $@ $< $(LDFLAGS)
	@echo "  LD  $@"

clean:
	rm -f $(TARGET)
	@echo "  clean done"

install: $(TARGET)
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(TARGET) $(DESTDIR)/usr/local/bin/$(TARGET)
	@echo "  install complete"
