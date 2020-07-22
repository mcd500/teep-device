
out-dir ?= .

all: $(out-dir)/teep-broker-app

$(out-dir)/teep-broker-app: main.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc  $(APP_CFLAGS) $(APP_LDFLAGS) $^ -o $@ $(APP_LIBS)

clean:
	rm -f $(out-dir)/teep-broker-app
