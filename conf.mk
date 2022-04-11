ifeq ($(TEE),)
else
BUILD = $(TOPDIR)/build/$(TEE)
include $(TOPDIR)/platform/conf-$(TEE).mk
endif

export HELLO_TA_UUID  ?= 8d82573a-926d-4754-9353-32dc29997f74
export TEE_AGENT_UUID ?= 68373894-5bb3-403c-9eec-3114a1f5d3fc

