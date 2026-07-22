CC      := gcc
SDKDIR := ../../sdk
CFLAGS  := -Wall -Wextra -std=c11 -O2 -pthread -Wno-format-truncation -I$(SDKDIR)/include -I$(SDKDIR)
LDFLAGS := -pthread -L$(SDKDIR) -lai-sdk
TARGET  := ai-intent-classifier

INCDIR  := include

MODULES := config protocol
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
	rm -f tests/*.o tests/test_classifier
	@echo "  clean done"

test: $(TARGET)
	@echo "--- Intent Classifier Unit Tests ---"
	@gcc $(CFLAGS) -I. -I$(INCDIR) tests/test_classifier.c config/config.o protocol/protocol.o -o tests/test_classifier $(LDFLAGS)
	@./tests/test_classifier
	@rm -f tests/test_classifier

install: $(TARGET)
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(TARGET) $(DESTDIR)/usr/local/bin/$(TARGET)
	install -d $(DESTDIR)/etc
	install -m 644 config.toml.example $(DESTDIR)/etc/ai-intent-classifier.toml
	@echo "  install complete"
