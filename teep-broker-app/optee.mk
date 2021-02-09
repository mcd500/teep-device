
out-dir ?= .

APP_CFLAGS += -I

all: $(out-dir)/teep-broker-app

$(out-dir)/teep-broker.o: teep-broker.c
$(out-dir)/http-lws.o: http-lws.c

$(out-dir)/teep-broker-app: $(out-dir)/teep-broker.o $(out-dir)/http-lws.o
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc $(APP_CFLAGS) $(APP_LDFLAGS) $^ -lteec -o $@ $(APP_LIBS)

$(out-dir)/%.o: %.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc  $(APP_CFLAGS) $< -c -o $@

clean:
	rm -f $(out-dir)/teep-broker-app
	rm -f $(out-dir)/*.o
