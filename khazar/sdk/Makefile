CC       = gcc
AR       = ar
CFLAGS   = -std=c11 -Wall -Wextra -O2 -pthread -fPIC -Wno-format-truncation
LDFLAGS  = -pthread
TARGET   = libai-sdk.a
PREFIX   = /usr/local

MODULES  = json logger protocol ipc agent events
SRCS     = $(foreach m,$(MODULES),$m/$m.c)
OBJS     = $(SRCS:.c=.o)
INCLUDES = -I. -I./include

TEST_CFLAGS  = $(CFLAGS)
TEST_LDFLAGS = $(LDFLAGS)

.PHONY: all clean test install uninstall

all: $(TARGET)
	@echo "=== ai-sdk build complete ==="

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^
	@echo "  AR  $@"

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<
	@echo "  CC  $<"

clean:
	rm -f $(OBJS) $(TARGET)
	rm -f tests/test_json tests/test_protocol tests/test_ipc tests/test_agent tests/test_events
	rm -f tests/*.o
	@echo "  clean done"

test: $(TARGET)
	@echo "--- Running Unit Tests ---"
	@$(CC) $(TEST_CFLAGS) $(INCLUDES) tests/test_json.c    -o tests/test_json    $(TARGET) $(TEST_LDFLAGS) && ./tests/test_json
	@$(CC) $(TEST_CFLAGS) $(INCLUDES) tests/test_protocol.c -o tests/test_protocol $(TARGET) $(TEST_LDFLAGS) && ./tests/test_protocol
	@$(CC) $(TEST_CFLAGS) $(INCLUDES) tests/test_ipc.c     -o tests/test_ipc     $(TARGET) $(TEST_LDFLAGS) && ./tests/test_ipc
	@$(CC) $(TEST_CFLAGS) $(INCLUDES) tests/test_agent.c   -o tests/test_agent   $(TARGET) $(TEST_LDFLAGS) && ./tests/test_agent
	@$(CC) $(TEST_CFLAGS) $(INCLUDES) tests/test_events.c  -o tests/test_events  $(TARGET) $(TEST_LDFLAGS) && ./tests/test_events
	@echo "--- All tests passed ---"

install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/lib
	install -m 644 $(TARGET) $(DESTDIR)$(PREFIX)/lib/$(TARGET)
	install -d $(DESTDIR)$(PREFIX)/include/ai-sdk
	install -m 644 include/common.h     $(DESTDIR)$(PREFIX)/include/ai-sdk/
	install -m 644 include/errors.h     $(DESTDIR)$(PREFIX)/include/ai-sdk/
	install -m 644 json/json.h          $(DESTDIR)$(PREFIX)/include/ai-sdk/
	install -m 644 logger/logger.h      $(DESTDIR)$(PREFIX)/include/ai-sdk/
	install -m 644 protocol/protocol.h  $(DESTDIR)$(PREFIX)/include/ai-sdk/
	install -m 644 ipc/ipc.h            $(DESTDIR)$(PREFIX)/include/ai-sdk/
	install -m 644 agent/agent.h        $(DESTDIR)$(PREFIX)/include/ai-sdk/
	install -m 644 events/events.h      $(DESTDIR)$(PREFIX)/include/ai-sdk/
	@echo "  install complete"

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/lib/$(TARGET)
	rm -rf $(DESTDIR)$(PREFIX)/include/ai-sdk
	@echo "  uninstall complete"
