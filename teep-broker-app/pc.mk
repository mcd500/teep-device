
out-dir ?= .

all: $(out-dir)/teep-broker-app

CFLAGS = $(APP_CFLAGS) -I../teep-agent-ta

$(out-dir)/teep-broker-app: pc-main.c teep-broker.c ../libteep/lib/main.c ../teep-agent-ta/ta-store.c ../teep-agent-ta/teep_message.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc  $(CFLAGS) $(APP_LDFLAGS) $^ -o $@ $(APP_LIBS)

clean:
	rm -f $(out-dir)/teep-broker-app
