
out-dir ?= .

all: $(out-dir)/teep-broker-app

$(out-dir)/teep-broker-app: keystone-main.cpp
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)g++  $(APP_CFLAGS) $(APP_LDFLAGS) $^ -o $@ $(APP_LIBS) -pthread

clean:
	rm -f $(out-dir)/teep-broker-app
