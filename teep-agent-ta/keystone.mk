CFLAGS += $(TA_CFLAGS)

LDFLAGS = $(TA_LDFLAGS)
LIBS = $(TA_LIBS) -lgcc

out-dir ?= .

all: $(out-dir)/teep-agent-ta

$(out-dir)/%.o: %.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $< -o $@

$(out-dir)/teep-agent-ta.o: teep-agent-ta.c teep-agent-ta.h ta-store.h
$(out-dir)/ta-store.o: ta-store.c teep-agent-ta.h ta-store.h

$(out-dir)/teep-agent-ta: $(out-dir)/teep-agent-ta.o $(out-dir)/ta-store.o $(out-dir)/tools.o
	$(CROSS_COMPILE)gcc -nostdlib -o $@ -static $^ $(LDFLAGS) $(LIBS)


