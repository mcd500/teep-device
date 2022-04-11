BUILD ?= $(CURDIR)/build
SOURCE ?= $(CURDIR)/../..

include ../conf-optee.mk

all: libteep hello-tc teep rootfs

include ../common.mk

clean: clean-hello-tc
	rm -rf $(BUILD)

rootfs: libteep hello-tc teep
	rm -rf $(BUILD)/rootfs-ext
	mkdir -p $(BUILD)/rootfs-ext/root/teep-device
	mkdir -p $(BUILD)/rootfs-ext/lib/optee_armtz
	mkdir -p $(BUILD)/rootfs-ext/usr/lib
	install $(SOURCE)/hello-tc/build-optee/App-optee $(BUILD)/rootfs-ext/root/teep-device/hello-app
	install $(BUILD)/teep-agent-ta/$(TEE_AGENT_UUID).ta $(BUILD)/rootfs-ext/lib/optee_armtz
	install $(BUILD)/teep-broker-app/teep-broker-app $(BUILD)/rootfs-ext/root/teep-device
	install $(BUILD)/libteep/ree/mbedtls/library/lib* $(BUILD)/rootfs-ext/usr/lib
	install $(BUILD)/libteep/ree/libwebsockets/lib/lib* $(BUILD)/rootfs-ext/usr/lib

	cp $(OPTEE_DIR)/out-br/images/rootfs.cpio.gz $(BUILD)/rootfs.cpio.gz
	cd $(BUILD)/rootfs-ext; \
		find . | cpio -o -H newc -R root:root | gzip -c9 >>$(BUILD)/rootfs.cpio.gz

run-qemu:
	cd $(OPTEE_DIR)/out/bin && \
		$(OPTEE_DIR)/qemu/aarch64-softmmu/qemu-system-aarch64 \
			-nographic \
			-serial mon:stdio -serial file:serial1.log \
			-smp 2 \
			-machine virt,secure=on -cpu cortex-a57 \
			-d unimp -semihosting-config enable,target=native \
			-m 1057 \
			-bios bl1.bin \
			-initrd $(BUILD)/rootfs.cpio.gz \
			-kernel Image -no-acpi \
			-append 'console=ttyAMA0,38400 keep_bootcon root=/dev/vda2' \
			$(QEMU_EXTRA_ARGS)

TAM_URL ?= http://tamproto_tam_api_1:8888

test: upload-hello-tc-download
	cd $(OPTEE_DIR)/out/bin && \
		QEMU=$(OPTEE_DIR)/qemu/aarch64-softmmu/qemu-system-aarch64 \
		QEMU_SMP=2 \
		TAM_URL=$(TAM_URL) \
		ROOTFS=$(BUILD)/rootfs.cpio.gz \
		expect $(CURDIR)/script/test.expect
	cat $(OPTEE_DIR)/out/bin/serial1.log

