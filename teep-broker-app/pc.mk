
out-dir ?= .

all: $(out-dir)/teep-broker-app

CFLAGS = $(APP_CFLAGS) -I../teep-agent-ta

$(out-dir)/pc-main.o: pc-main.c
$(out-dir)/teep-broker.o: teep-broker.c
$(out-dir)/libteep.o: ../libteep/lib/libteep.c
$(out-dir)/ta-store.o: ../teep-agent-ta/ta-store.c ../teep-agent-ta/ta-store.h ../teep-agent-ta/teep-agent-ta.h
$(out-dir)/teep_message.o: ../teep-agent-ta/teep_message.c ../teep-agent-ta/teep_message.h ../teep-agent-ta/teep-agent-ta.h

OBJ = \
	$(out-dir)/pc-main.o \
	$(out-dir)/teep-broker.o \
	$(out-dir)/libteep.o \
	$(out-dir)/ta-store.o \
	$(out-dir)/teep_message.o

$(out-dir)/teep-broker-app: $(OBJ)
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc  $(CFLAGS) $(APP_LDFLAGS) $^ -o $@ $(APP_LIBS)

$(out-dir)/%.o: %.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc $(CFLAGS) $< -c -o $@

$(out-dir)/%.o: ../libteep/lib/%.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc $(CFLAGS) $< -c -o $@

$(out-dir)/%.o: ../teep-agent-ta/%.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc $(CFLAGS) $< -c -o $@

clean:
	rm -f $(out-dir)/teep-broker-app
