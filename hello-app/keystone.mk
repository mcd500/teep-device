
out-dir ?= .

all: $(out-dir)/hello-app

$(out-dir)/hello-app: main.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc  $(APP_CFLAGS) $(APP_LDFLAGS) $^ -o $@ $(APP_LIBS)

clean:
	rm -f $(out-dir)/hello-app
