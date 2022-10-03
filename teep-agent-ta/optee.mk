TOPDIR = $(CURDIR)/..
include $(TOPDIR)/conf.mk

export TA_CFLAGS = $(TEE_CFLAGS) $(DFLAGS) \
	-nostdinc -DPLAT_OPTEE=1  \
	-Wall \
	-Wno-overlength-strings \
	-I$(TA_DEV_KIT_DIR) \
	-I$(TOPDIR)/include \
	-I$(BUILD)/tee/libwebsockets/include \
	-I$(TOPDIR)/submodule/QCBOR/inc \
	-I$(TOPDIR)/lib/include

export TA_LDFLAGS = \
	-L$(BUILD)/tee/QCBOR \
	-L$(BUILD)/lib

CPPFLAGS += -DTEE_TA
CFLAGS += $(TA_CFLAGS)
LDADD += $(TA_LDFLAGS) -lteep -lqcbor -lteesuit -lt_cose -lteecbor -lteelog

BINARY = $(TEE_AGENT_UUID)

-include $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk

.PHONY: clean
clean:
	rm -rf *.o .*.o.* *.dmp *.elf *.map $(BINARY).ta ta.lds .ta.ld.d

.PHONY: clean_ta_file
clean_ta_file:
	rm -f $(BINARY).ta
