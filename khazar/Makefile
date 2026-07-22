CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -O2 -pthread -Wno-format-truncation
LDFLAGS := -pthread -lm

SDK_DIR     := sdk
SDK_LIB     := $(SDK_DIR)/libai-sdk.a
COMPONENTS  := orchestrator rule-engine policy-engine model-runtime intent-classifier
AGENTS      := desktop-agent package-agent network-agent power-agent audio-agent

.PHONY: all sdk components agents clean test install

all: sdk components agents
	@echo "=== Khazar System - full build complete ==="

sdk:
	@echo "--- Building SDK ---"
	@$(MAKE) -C $(SDK_DIR) --no-print-directory

components: sdk
	@for c in $(COMPONENTS); do \
		echo "--- Building component: $$c ---"; \
		$(MAKE) -C components/$$c --no-print-directory || exit 1; \
	done

agents: sdk
	@for a in $(AGENTS); do \
		echo "--- Building agent: $$a ---"; \
		$(MAKE) -C agents/$$a --no-print-directory || exit 1; \
	done

test: all
	@echo "=== Running SDK Tests ==="
	@$(MAKE) -C $(SDK_DIR) test --no-print-directory
	@echo ""
	@echo "=== Running Component Tests ==="
	@for c in $(COMPONENTS); do \
		if [ -f components/$$c/Makefile ] && grep -q "^test:" components/$$c/Makefile 2>/dev/null; then \
			echo "--- Testing: $$c ---"; \
			$(MAKE) -C components/$$c test --no-print-directory || true; \
		fi; \
	done

clean:
	@$(MAKE) -C $(SDK_DIR) clean --no-print-directory
	@for c in $(COMPONENTS); do \
		$(MAKE) -C components/$$c clean --no-print-directory 2>/dev/null || true; \
	done
	@for a in $(AGENTS); do \
		$(MAKE) -C agents/$$a clean --no-print-directory 2>/dev/null || true; \
	done
	@rm -f tests/test_runner
	@echo "  clean done"

install: all
	install -d $(DESTDIR)/usr/local/bin
	for c in $(COMPONENTS); do \
		if [ -f components/$$c/ai-$$c ]; then \
			install -m 755 components/$$c/ai-$$c $(DESTDIR)/usr/local/bin/; \
		fi; \
	done
	for a in $(AGENTS); do \
		if [ -f agents/$$a/ai-$$a ]; then \
			install -m 755 agents/$$a/ai-$$a $(DESTDIR)/usr/local/bin/; \
		fi; \
	done
	install -d $(DESTDIR)/usr/local/lib
	install -m 644 $(SDK_LIB) $(DESTDIR)/usr/local/lib/
	install -d $(DESTDIR)/usr/local/include/ai-sdk
	install -m 644 $(SDK_DIR)/include/*.h $(DESTDIR)/usr/local/include/ai-sdk/
	install -m 644 $(SDK_DIR)/*/*.h $(DESTDIR)/usr/local/include/ai-sdk/
	@echo "  install complete"
