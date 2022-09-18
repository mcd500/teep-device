TOPDIR = $(CURDIR)/..
include $(TOPDIR)/conf.mk

.PHONY: all
all: $(out-dir)/agent.a

CFLAGS = \
	-I$(TOPDIR)/include \
	-I$(TOPDIR)/suit/include \
	-I$(TOPDIR)/libteep/lib \
	-I$(TOPDIR)/platform/pc/include \
	-I$(BUILD)/tee/QCBOR/inc \
	-DPLAT_PC

$(out-dir)/%.o: %.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $< -o $@

$(out-dir)/teep-agent-ta.o: teep-agent-ta.c teep-agent-ta.h ta-store.h
$(out-dir)/ta-store.o: ta-store.c teep-agent-ta.h ta-store.h

$(out-dir)/agent.a: $(out-dir)/teep-agent-ta.o $(out-dir)/ta-store.o $(out-dir)/tools.o
	ar cr $@ $^
