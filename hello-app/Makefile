TARGET ?= hello-app hello-app-keystone
TEE_REF_TA_DIR = $(CURDIR)/../..
ifndef KEYSTONE_SDK_DIR
export KEYSTONE_SDK_DIR = $(KEYSTONE_DIR)/sdk
endif

.PHONY: all
all: $(TARGET)

.PHONY: keystone
	hello-app-keystone

hello-app: main.c
	$(CROSS_COMPILE)gcc -I$(TA_DEV_KIT_DIR)/host_include $(INCLUDES) -L../../optee_client/out/libteec $(LDTEEC) $^ -lteec -o $@

KEYSTONE_INCLUDES = \
-I$(KEYSTONE_SDK_DIR)/lib/host/include \
-I$(KEYSTONE_SDK_DIR)/lib/edge/include \
-I$(KEYSTONE_SDK_DIR)/lib/verifier \
-I$(TEE_REF_TA_DIR)/ref-ta/keystone/../../keyedge/target/include \
-I$(TEE_REF_TA_DIR)/ref-ta/keystone/../../keyedge/flatcc/include \
-I$(TEE_REF_TA_DIR)/ref-ta/keystone//Edge \
-I./ \
-I$(TEE_REF_TA_DIR)/ref-ta/keystone/App/../../profiler

main-keystone.o: main.c
	$(CROSS_COMPILE)gcc $< -o $@ -DKEYSTONE -DAPP_VERBOSE $(KEYSTONE_INCLUDES)

hello-app-keystone: main-keystone.o \
App_ocalls.o \
Enclave_u.o \
$(KEYSTONE_SDK_DIR)/lib/libkeystone-host.a \
$(KEYSTONE_SDK_DIR)/sdk/lib/libkeystone-edge.a \
$(KEYSTONE_SDK_DIR)/sdk/lib/libkeystone-verifier.a \
$(TEE_REF_TA_DIR)/ref-ta/keystone/../../keyedge/lib/flatccrt.a \
$(TEE_REF_TA_DIR)/ref-ta/keystone/App/../../profiler/libhostprofiler.a
	$(CROSS_COMPILE)gcc -static $^ -o $@

.PHONY: clean
clean:
	rm -f $(TARGET)

.PHONY: update
update:
	scp $(TARGET) root@${HIKEY_IP}:/usr/bin

.PHONY: keystone
keystone:
	@echo TODO
