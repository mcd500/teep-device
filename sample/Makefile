TOPDIR = $(CURDIR)/..
include $(TOPDIR)/conf.mk

.PHONY: rootfs
rootfs: rootfs-$(TEE)

.PHONY: run-qemu
run-qemu: run-qemu-$(TEE)

.PHONY: run-session
run-session: run-session-$(TEE)

.PHONY: scripts
scripts:
	mkdir -p $(BUILD)/scripts
	for i in ita.sh showtamurl.sh env.sh ; do \
		sed -e 's!@TAM_URL@!$(TAM_URL)!' \
			-e 's!@PLAT@!$(PLAT)!' \
			rootfs/scripts/$$i.in >$(BUILD)/scripts/$$i; \
		chmod +x $(BUILD)/scripts/$$i; \
	done

.PHONY: rootfs-keystone
rootfs-keystone: scripts
	cp $(KEYSTONE_DIR)/build/buildroot.build/images/rootfs.ext2 $(BUILD)/rootfs.ext2
	e2mkdir -O root -G root $(BUILD)/rootfs.ext2:/root/teep-device
	e2cp -O root -G root -p $(BUILD)/hello-tc/Enclave $(BUILD)/rootfs.ext2:/root/teep-device/hello-ta
	e2cp -O root -G root -p $(BUILD)/hello-tc/App-keystone $(BUILD)/rootfs.ext2:/root/teep-device/hello-app
	e2cp -O root -G root -p $(BUILD)/agent/teep-agent-ta $(BUILD)/rootfs.ext2:/root/teep-device
	e2cp -O root -G root -p $(BUILD)/broker/teep-broker-app $(BUILD)/rootfs.ext2:/root/teep-device
	e2cp -O root -G root -p $(BUILD)/scripts/ita.sh $(BUILD)/rootfs.ext2:/root/teep-device
	e2cp -O root -G root -p $(BUILD)/scripts/env.sh $(BUILD)/rootfs.ext2:/root/teep-device
	e2cp -O root -G root -p $(BUILD)/scripts/showtamurl.sh $(BUILD)/rootfs.ext2:/root/teep-device
	e2cp -O root -G root -p $(KEYSTONE_SDK_DIR)/runtime/eyrie-rt $(BUILD)/rootfs.ext2:/root/teep-device
	e2cp -O root -G root -p $(BUILD)/ree/mbedtls/library/lib* $(BUILD)/rootfs.ext2:/usr/lib
	e2cp -O root -G root -p $(BUILD)/ree/libwebsockets/lib/lib* $(BUILD)/rootfs.ext2:/usr/lib

.PHONY: rootfs-optee
rootfs-optee: scripts
	rm -rf $(BUILD)/rootfs-ext
	mkdir -p $(BUILD)/rootfs-ext/root/teep-device
	mkdir -p $(BUILD)/rootfs-ext/lib/optee_armtz
	mkdir -p $(BUILD)/rootfs-ext/usr/lib
	install $(BUILD)/hello-tc/App-optee $(BUILD)/rootfs-ext/root/teep-device/hello-app
	install $(BUILD)/agent/$(TEE_AGENT_UUID).ta $(BUILD)/rootfs-ext/lib/optee_armtz
	install $(BUILD)/broker/teep-broker-app $(BUILD)/rootfs-ext/root/teep-device
	install $(BUILD)/scripts/ita.sh $(BUILD)/rootfs-ext/root/teep-device
	install $(BUILD)/scripts/env.sh $(BUILD)/rootfs-ext/root/teep-device
	install $(BUILD)/scripts/showtamurl.sh $(BUILD)/rootfs-ext/root/teep-device
	install $(BUILD)/ree/mbedtls/library/lib* $(BUILD)/rootfs-ext/usr/lib
	install $(BUILD)/ree/libwebsockets/lib/lib* $(BUILD)/rootfs-ext/usr/lib

	cp $(OPTEE_DIR)/out-br/images/rootfs.cpio.gz $(BUILD)/rootfs.cpio.gz
	cd $(BUILD)/rootfs-ext; \
		find . | cpio -o -H newc -R root:root | gzip -c9 >>$(BUILD)/rootfs.cpio.gz

.PHONY: rootfs-sgx
rootfs-sgx:
.PHONY: rootfs-pc
rootfs-pc:

.PHONY: run-qemu-optee
run-qemu-optee:
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

.PHONY: run-qemu-keystone
run-qemu-keystone:
	qemu-system-riscv64 \
		-m 4G \
		-bios $(KEYSTONE_DIR)/build/bootrom.build/bootrom.bin \
		-nographic \
		-machine virt \
		-kernel $(KEYSTONE_DIR)/build/sm.build/platform/generic/firmware/fw_payload.elf \
		-append "console=ttyS0 ro root=/dev/vda cma=256M@0x00000000C0000000" \
		-device virtio-blk-device,drive=hd0 -drive file=$(BUILD)/rootfs.ext2,format=raw,id=hd0 \
		-netdev user,id=net0,net=192.168.100.1/24,dhcpstart=192.168.100.128,hostfwd=tcp::10032-:22 \
		-device virtio-net-device,netdev=net0 \
		-device virtio-rng-pci

.PHONY: run-qemu-sgx
run-qemu-sgx:
	@echo 'sgx does not need qemu'
	false

.PHONY: run-qemu-pc
	@echo 'pc does not need qemu'
	false

TAM_URL ?= http://tamproto_tam_api_1:8888

upload-hello-tc-download:
	$(MAKE) -C $(TOPDIR)/hello-tc/build-$(PLAT) SOURCE=$(TOPDIR)/hello-tc upload-download-manifest

upload-hello-tc-embed:
	$(MAKE) -C $(TOPDIR)/hello-tc/build-$(PLAT) SOURCE=$(TOPDIR)/hello-tc upload-embed-manifest

run-session-optee: upload-hello-tc-download
	cd $(OPTEE_DIR)/out/bin && \
		QEMU=$(OPTEE_DIR)/qemu/aarch64-softmmu/qemu-system-aarch64 \
		QEMU_SMP=2 \
		TAM_URL=$(TAM_URL) \
		ROOTFS=$(BUILD)/rootfs.cpio.gz \
		expect $(CURDIR)/session/test-optee.expect
	cat $(OPTEE_DIR)/out/bin/serial1.log
	! fgrep 'ERR:' $(OPTEE_DIR)/out/bin/serial1.log
	fgrep 'Hello TEEP from TEE!' $(OPTEE_DIR)/out/bin/serial1.log

run-session-keystone: upload-hello-tc-download
	TAM_URL=$(TAM_URL) BUILD=$(BUILD) \
		expect ./session/test-keystone.expect | \
		tee $(BUILD)/keystone.log
	ls -l
	echo $(BUILD) && ls -l $(BUILD)
	! fgrep 'ERR:' $(BUILD)/keystone.log
	fgrep 'Hello TEEP from TEE!' $(BUILD)/keystone.log

run-session-sgx: upload-hello-tc-download
	cd $(BUILD)/agent && \
		../broker/teep-broker-app --tamurl $(TAM_URL)/api/tam_cbor

run-session-pc: upload-hello-tc-embed
	cd $(BUILD)/broker && \
		./teep-broker-app --tamurl $(TAM_URL)/api/tam_cbor | tee $(BUILD)/pctest.log
	! fgrep 'ERR:' $(BUILD)/pctest.log
	fgrep 'store component' $(BUILD)/pctest.log



