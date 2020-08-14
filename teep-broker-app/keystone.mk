
out-dir ?= .

all: $(out-dir)/teep-broker-app

$(out-dir)/teep-broker-app: keystone-main.cpp $(out-dir)/teep-broker.o
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)g++  $(APP_CFLAGS) $(APP_LDFLAGS) $^ -o $@ $(APP_LIBS) -pthread

$(out-dir)/%.o: %.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc  $(APP_CFLAGS) $(APP_LDFLAGS) $< -c -o $@

clean:
	rm -f $(out-dir)/teep-broker-app
