TOPDIR = $(CURDIR)/..
include $(TOPDIR)/conf.mk

CFLAGS += $(TEE_CFLAGS) $(DFLAGS) \
	-Wall -fno-builtin-printf -DEDGE_IGNORE_EGDE_RESULT \
	-I. \
	-I$(TOPDIR)/include \
	-I$(TOPDIR)/submodule/QCBOR/inc \
	-I$(TOPDIR)/lib/include \
	-DPLAT_KEYSTONE

LDFLAGS = $(TEE_LDFLAGS) \
	-L$(BUILD)/tee/libwebsockets/lib \
	-L$(BUILD)/tee/QCBOR \
	-L$(BUILD)/lib

LIBS = $(TEE_LIBS) \
	-lteep \
	-lteelog \
	-lqcbor \
	-lteesuit \
	-lt_cose \
	-lteecbor \
	-lmbedtls \
	-lgcc

.PHONY: all
all: patch_strlen $(out-dir)/teep-agent-ta

# fixing untypical strlen(char* str); to typical strlen(const char* str);
# to avoid many warnings
STRINGHS = $(KEYSTONE_DIR)/sdk/include/app/string.h \
	   $(KEYSTONE_DIR)/sdk/build64/include/app/string.h
.PHONY: patch_strlen
patch_strlen: $(STRINGHS)
	sed '/^strlen/ s/(char\*/(const\ char\*/' -i $^

$(out-dir)/%.o: %.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $< -o $@

$(out-dir)/teep-agent-ta.o: teep-agent-ta.c teep-agent-ta.h ta-store.h
$(out-dir)/ta-store.o: ta-store.c teep-agent-ta.h ta-store.h

$(out-dir)/teep-agent-ta: $(out-dir)/teep-agent-ta.o $(out-dir)/ta-store.o $(out-dir)/tools.o
	$(CROSS_COMPILE)gcc -nostdlib -o $@ -static $^ $(LDFLAGS) $(LIBS)


