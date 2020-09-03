CFLAGS += $(TA_CFLAGS)
LDFLAGS = $(TA_LDFLAGS)
LIBS = $(TA_LIBS) -lgcc

out-dir ?= .

all: $(out-dir)/hello-ta

$(out-dir)/hello-ta.o: hello-ta.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $< -o $@

$(out-dir)/hello-ta: $(out-dir)/hello-ta.o
	$(CROSS_COMPILE)gcc -nostdlib -o $@ -static $^ $(LDFLAGS) $(LIBS)
