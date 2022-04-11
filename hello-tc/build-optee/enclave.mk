TOPDIR = $(CURDIR)/../..
include $(TOPDIR)/conf.mk

BINARY = $(HELLO_TA_UUID)
CPPFLAGS = $(TEE_CFLAGS) \
	-I$(TAREF_DIR)/build/include/api \
	-Dtee_printf=printf

-include $(TA_DEV_DIR)/mk/ta_dev_kit.mk
