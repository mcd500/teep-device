CFLAGS += $(TA_CFLAGS)

LDFLAGS = $(TA_LDFLAGS)
LIBS = $(TA_LIBS) -lgcc

out-dir ?= .

all: $(out-dir)/teep-agent-ta

$(out-dir)/%.o: %.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $< -o $@

$(out-dir)/teep-agent-ta.o: teep-agent-ta.c
$(out-dir)/teep_message.o: teep_message.c
$(out-dir)/ta-store.o: ta-store.c

$(out-dir)/teep-agent-ta: $(out-dir)/teep-agent-ta.o $(out-dir)/teep_message.o $(out-dir)/ta-store.o $(out-dir)/tools.o
	$(CROSS_COMPILE)gcc -nostdlib -o $@ -static $^ $(LDFLAGS) $(LIBS)


