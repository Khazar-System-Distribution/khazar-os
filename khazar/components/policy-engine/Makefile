CC      := gcc
SDKDIR := ../../sdk
CFLAGS  := -Wall -Wextra -std=c11 -O2 -pthread -Wno-format-truncation -I$(SDKDIR)/include -I$(SDKDIR)
LDFLAGS := -pthread -L$(SDKDIR) -lai-sdk
TARGET  := ai-policy-engine

INCDIR  := include

MODULES := config policy protocol
SRCS    := main.c $(foreach m,$(MODULES),$m/$m.c)
OBJS    := $(SRCS:.c=.o)

.PHONY: all clean test install uninstall

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
	rm -f tests/test_policy tests/*.o
	@echo "  clean done"

test: $(TARGET)
	@echo "--- Policy Engine Unit Tests ---"
	@gcc $(CFLAGS) -I. -I$(INCDIR) tests/test_policy.c config/config.o policy/policy.o protocol/protocol.o -o tests/test_policy $(LDFLAGS)
	@./tests/test_policy
	@rm -f tests/test_policy

install: $(TARGET)
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(TARGET) $(DESTDIR)/usr/local/bin/$(TARGET)
	install -d $(DESTDIR)/etc/ai-policy-engine/rules
	install -m 644 config.toml.example $(DESTDIR)/etc/ai-policy-engine.toml
	install -m 644 rules/defaults/*.toml $(DESTDIR)/etc/ai-policy-engine/rules/
	@echo "  install complete"

uninstall:
	rm -f $(DESTDIR)/usr/local/bin/$(TARGET)
	rm -f $(DESTDIR)/etc/ai-policy-engine.toml
	rm -rf $(DESTDIR)/etc/ai-policy-engine
	@echo "  uninstall complete"
