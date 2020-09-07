CPPFLAGS += -DTEE_TA
CFLAGS += $(TA_CFLAGS)
LDADD += $(TA_LDFLAGS)
BINARY = $(TEE_AGENT_UUID)

libnames += websockets

-include $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk
