
out-dir ?= .

all: $(out-dir)/hello-app

$(out-dir)/hello-app: keystone-main.cpp
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)g++  $(APP_CFLAGS) $(APP_LDFLAGS) $^ -o $@ $(APP_LIBS)

clean:
	rm -f $(out-dir)/hello-app
