CFLAGS += $(TA_CFLAGS)

LDFLAGS = $(TA_LDFLAGS)

out-dir ?= .

all: $(out-dir)/teep-agent-ta.signed.so

$(out-dir)/%.o: %.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $< -o $@

$(out-dir)/teep-agent-ta.o: teep-agent-ta.c teep-agent-ta.h ta-store.h
$(out-dir)/ta-store.o: ta-store.c teep-agent-ta.h ta-store.h

$(out-dir)/teep-agent-ta.so: $(out-dir)/teep-agent-ta.o $(out-dir)/ta-store.o $(out-dir)/tools.o
	$(CROSS_COMPILE)gcc -nostdlib -o $@ -static $^ $(LDFLAGS)

$(out-dir)/teep-agent-ta.signed.so: $(out-dir)/teep-agent-ta.so
	$(ENCLAVE_SIGNER_BIN) sign \
		-key $(ENCLAVE_PEM) \
		-enclave $< \
		-out $@ \
		-config $(ENCLAVE_CONFIG_FILE)
