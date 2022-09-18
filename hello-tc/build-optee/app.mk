TOPDIR = $(CURDIR)/../..
include $(TOPDIR)/conf.mk

out-dir = $(BUILD)/hello-tc

.PHONY: all
all: $(out-dir)/App-optee

$(out-dir)/App-optee: $(out-dir)/App-optee.o
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc -o $@ $^ $(REE_LDFLAGS) $(REE_LIBS)

$(out-dir)/App-optee.o: ../App-optee.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc $(REE_CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -f $(out-dir)/App-optee.o $(out-dir)/App-optee
