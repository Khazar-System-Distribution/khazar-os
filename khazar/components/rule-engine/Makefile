CC       = gcc
SDKDIR = ../../sdk
CFLAGS   = -std=c11 -Wall -Wextra -O2 -pthread -I$(SDKDIR)/include -I$(SDKDIR)
LDFLAGS  = -pthread -L$(SDKDIR) -lai-sdk
BINARY   = ai-rule-engine
SOCK     = /tmp/ai-rule.sock
CFG      = /tmp/ai-rule-interactive.toml

SRCS     = main.c \
           config/config.c \
           protocol/protocol.c \
           metrics/metrics.c \
           cache/cache.c \
           matcher/matcher.c \
           tokens/tokens.c \
           intent/intent.c \
           alias/alias.c \
           fuzzy/fuzzy.c \
           rules/rules.c

OBJS     = $(SRCS:.c=.o)
INCLUDES = -I. -I./include -I$(SDKDIR)/include -I$(SDKDIR)

all: $(BINARY)

$(BINARY): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "[OK] $(BINARY) built successfully"

%.o: %.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(BINARY) $(OBJS) tools/interactive tools/ai-cli
	rm -f tests/*.o tests/test_pipeline tests/test_integration
	rm -f $(CFG)

tools/interactive: tools/interactive.c
	$(CC) -std=c11 -Wall -O2 -o $@ $<

tools/ai-cli: tools/ai-cli.c
	$(CC) -std=c11 -Wall -O2 -o $@ $<

# Interaktiv test — rule engine-i avtomatik başladır
interactive: $(BINARY) tools/interactive
	@echo "[*] Rule Engine başladılır..."
	@rm -f $(SOCK)
	@echo 'socket_path="$(SOCK)"' > $(CFG)
	@echo 'max_connections=64' >> $(CFG)
	@echo 'enable_cache=true' >> $(CFG)
	@echo 'enable_regex=true' >> $(CFG)
	@echo 'enable_tokens=true' >> $(CFG)
	@echo 'enable_intent_table=true' >> $(CFG)
	@echo 'enable_alias=true' >> $(CFG)
	@echo 'enable_fuzzy=false' >> $(CFG)
	@echo 'daemonize=false' >> $(CFG)
	@echo 'log_level=4' >> $(CFG)
	@./$(BINARY) $(CFG) > /dev/null 2>&1 &
	@sleep 0.5
	@for i in 1 2 3 4 5 6 7 8 9 10; do [ -S $(SOCK) ] && break; sleep 0.2; done
	@./tools/interactive $(SOCK); \
	kill %1 2>/dev/null; \
	rm -f $(SOCK) $(CFG); \
	echo "[*] Rule Engine dayandirildi."

# Full chain CLI — orchestator üzərindən
cli: tools/ai-cli
	@./tools/ai-cli /tmp/ai-orch.sock

test: $(BINARY)
	@echo "--- Unit Tests ---"
	@gcc $(CFLAGS) $(INCLUDES) tests/test_pipeline.c config/config.o protocol/protocol.o metrics/metrics.o cache/cache.o matcher/matcher.o tokens/tokens.o intent/intent.o alias/alias.o fuzzy/fuzzy.o rules/rules.o -o tests/test_pipeline $(LDFLAGS)
	@./tests/test_pipeline
	@rm -f tests/test_pipeline

integration: $(BINARY)
	@echo "--- Integration Tests ---"
	@gcc $(CFLAGS) $(INCLUDES) tests/test_integration.c -o tests/test_integration $(LDFLAGS)
	@./tests/test_integration
	@rm -f tests/test_integration

install: $(BINARY)
	install -m 0755 $(BINARY) /usr/local/bin/$(BINARY)
	install -m 0644 config.toml.example /etc/ai-rule-engine.toml
	install -d /etc/ai-rule-engine/rules
	install -m 0644 rules/defaults/*.rules /etc/ai-rule-engine/rules/
	install -m 0644 ai-rule-engine.service /etc/systemd/system/
	@echo "[OK] Installed: binary=/usr/local/bin/ai-rule-engine config=/etc/ai-rule-engine.toml"

.PHONY: all clean test integration install interactive cli
