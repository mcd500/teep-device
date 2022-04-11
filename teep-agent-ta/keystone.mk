TOPDIR = $(CURDIR)/..
include $(TOPDIR)/conf.mk

CFLAGS = $(TEE_CFLAGS) \
	-Wall -fno-builtin-printf -DEDGE_IGNORE_EGDE_RESULT \
	-I. \
	-I$(TOPDIR)/include \
	-I$(BUILD)/tee/QCBOR/inc \
	-I$(TOPDIR)/suit/include \
	-I$(TOPDIR)/libteep/lib \
	-DPLAT_KEYSTONE

LDFLAGS = $(TEE_LDFLAGS) \
	-L$(BUILD)/tee/libwebsockets/lib \
	-L$(BUILD)/tee/QCBOR \
	-L$(BUILD)/tee/libteep \
	-L$(BUILD)/tee/suit/lib

LIBS = $(TEE_LIBS) \
	-lteep \
	-lqcbor \
	-lteesuit \
	-lgcc

all: $(out-dir)/teep-agent-ta

$(out-dir)/%.o: %.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $< -o $@

$(out-dir)/teep-agent-ta.o: teep-agent-ta.c teep-agent-ta.h ta-store.h
$(out-dir)/ta-store.o: ta-store.c teep-agent-ta.h ta-store.h

$(out-dir)/teep-agent-ta: $(out-dir)/teep-agent-ta.o $(out-dir)/ta-store.o $(out-dir)/tools.o
	$(CROSS_COMPILE)gcc -nostdlib -o $@ -static $^ $(LDFLAGS) $(LIBS)


