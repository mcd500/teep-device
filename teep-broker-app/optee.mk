
out-dir ?= .

all: $(out-dir)/teep-broker-app

$(out-dir)/teep-broker-app: optee-main.c teep-broker.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc $(APP_CFLAGS) $(APP_LDFLAGS) $^ -lteec -o $@ $(APP_LIBS)

clean:
	rm -f $(out-dir)/teep-broker-app
