CFG_TEE_TA_LOG_LEVEL ?= 3
CPPFLAGS += -DCFG_TEE_TA_LOG_LEVEL=$(CFG_TEE_TA_LOG_LEVEL)
CFLAGS += -DPLAT_OPTEE=1  \
	  -DAIST_TB_PROVISIONING_URL="\"$(AIST_TB_PROVISIONING_URL)\"" \
	  -DAIST_TB_TA_UUID_STRUCT="$(AIST_TB_TA_UUID_STRUCT)" \
	  -Wno-overlength-strings -I$(TA_DEV_KIT_DIR) -L$(TA_DEV_KIT_DIR)/lib

CROSS_COMPILE=${CROSS_COMPILE_TA}

export CROSS_COMPILE_HOST
export CROSS_COMPILE_TA
export TA_DEV_KIT_DIR
export CROSS_COMPILE
export AIST_TB_KERNEL_SRC_DIR
export AIST_TB_KERNEL_BUILD_DIR

GCC_VER=$(shell $(CROSS_COMPILE)gcc --version | head -n1 )

BINARY:=68373894-5bb3-403c-9eec-3114a1f5d3fc
export BINARY
LDFLAGS+=-L$(TA_DEV_KIT_DIR)/lib  --whole-archive -lwebsockets --no-whole-archive


-include $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk

$(BINARY).elf: $(TA_DEV_KIT_DIR)/lib/libwebsockets.a \
		$(TA_DEV_KIT_DIR)/lib/libmbedtls.a \
			*.c *.h

$(BINARY).ta:	$(BINARY).elf

#

.PHONY: all 
ifneq ($(wildcard $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk),)
all: $(BINARY).ta
else
all:
	@echo "$(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk does not exist. Is TA_DEV_KIT_DIR correctly defined?" && false
endif

.PHONY: clean
clean:
	rm -rf *.o .*.o.* *.cmd *.dmp *.elf *.map $(BINARY).ta .ta.ld.d ta.lds

update:
	scp *.ta root@${HIKEY_IP}:/lib/optee_armtz

.PHONY: clean_ta_file
clean_ta_file:
	rm -f $(BINARY).ta
