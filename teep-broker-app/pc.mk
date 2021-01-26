
out-dir ?= .

all: $(out-dir)/teep-broker-app

CFLAGS = $(APP_CFLAGS) -I../teep-agent-ta

$(out-dir)/teec-pc.o: teec-pc.c
$(out-dir)/teep-broker.o: teep-broker.c http.h
$(out-dir)/http-lws.o: http-lws.c http.h
$(out-dir)/libteep.o: ../libteep/lib/libteep.c ../libteep/lib/libteep.h ../libteep/lib/qcbor-ext.h
$(out-dir)/qcbor-ext.o: ../libteep/lib/qcbor-ext.c ../libteep/lib/qcbor-ext.h
$(out-dir)/ta-store.o: ../teep-agent-ta/ta-store.c ../teep-agent-ta/ta-store.h ../teep-agent-ta/teep-agent-ta.h
$(out-dir)/teep-agent-ta.o: ../teep-agent-ta/teep-agent-ta.c ../teep-agent-ta/ta-store.h ../teep-agent-ta/teep-agent-ta.h

OBJ = \
	$(out-dir)/teec-pc.o \
	$(out-dir)/teep-broker.o \
	$(out-dir)/http-lws.o \
	$(out-dir)/libteep.o \
	$(out-dir)/qcbor-ext.o \
	$(out-dir)/ta-store.o \
	$(out-dir)/teep-agent-ta.o

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
	rm -f $(out-dir)/*.o
