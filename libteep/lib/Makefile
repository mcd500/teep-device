TOPDIR = $(CURDIR)/../..
include $(TOPDIR)/conf.mk

B = $(BUILD)/tee/libteep

CFLAGS = \
	$(TEE_CFLAGS) \
	-I$(BUILD)/tee/QCBOR/inc

.PHONY: all
all: $(B)/libteep.a

$(B)/libteep.a: $(B)/libteep.o $(B)/qcbor-ext.o
	$(CROSS_COMPILE)ar cr $@ $^

$(B)/%.o: %.c
	@mkdir -p $(B)
	$(CROSS_COMPILE)gcc -c -o $@ $< $(CFLAGS)

$(B)/libteep.o: libteep.c libteep.h qcbor-ext.h
$(B)/qcbor-ext.o: qcbor-ext.c qcbor-ext.h
