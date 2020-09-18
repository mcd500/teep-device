
out-dir ?= .

all: $(out-dir)/teep-broker-app

$(out-dir)/keystone-main.o: keystone-main.cpp
$(out-dir)/teep-broker.o: teep-broker.c

$(out-dir)/teep-broker-app: $(out-dir)/keystone-main.o $(out-dir)/teep-broker.o
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)g++  $(APP_CFLAGS) $(APP_LDFLAGS) $^ -o $@ $(APP_LIBS) -pthread

$(out-dir)/%.o: %.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc  $(APP_CFLAGS) $< -c -o $@

$(out-dir)/%.o: %.cpp
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)g++  $(APP_CFLAGS) $< -c -o $@


clean:
	rm -f $(out-dir)/teep-broker-app
	rm -f $(out-dir)/teep-broker.o
