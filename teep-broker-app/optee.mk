
out-dir ?= .

all: $(out-dir)/teep-broker-app

$(out-dir)/optee-main.o: optee-main.c teep-broker.h
$(out-dir)/teep-broker.o: teep-broker.c teep-broker.h

$(out-dir)/teep-broker-app: $(out-dir)/optee-main.o $(out-dir)/teep-broker.o
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc $(APP_CFLAGS) $(APP_LDFLAGS) $^ -lteec -o $@ $(APP_LIBS)

$(out-dir)/%.o: %.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc  $(APP_CFLAGS) $< -c -o $@

clean:
	rm -f $(out-dir)/teep-broker-app
