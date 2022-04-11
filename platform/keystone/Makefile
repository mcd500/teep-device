export BUILD ?= $(CURDIR)/build
export SOURCE ?= $(CURDIR)/../..

include ../conf-keystone.mk

all: libteep hello-tc teep

include ../common.mk

clean: clean-hello-tc
	rm -rf $(BUILD)

prefix ?= $(KEYSTONE_DIR)/build/overlay

install: all
	test -n "$(prefix)"
	mkdir -p $(prefix)/root/teep-device
	install $(SOURCE)/hello-tc/build-keystone/Enclave $(prefix)/root/teep-device/hello-ta
	install $(SOURCE)/hello-tc/build-keystone/App-keystone $(prefix)/root/teep-device/hello-app
	install $(BUILD)/teep-agent-ta/teep-agent-ta $(prefix)/root/teep-device
	install $(BUILD)/teep-broker-app/teep-broker-app $(prefix)/root/teep-device
	install $(KEYSTONE_SDK_DIR)/runtime/eyrie-rt $(prefix)/root/teep-device
	mkdir -p $(prefix)/usr/lib
	install $(BUILD)/libteep/ree/mbedtls/library/lib* $(prefix)/usr/lib
	install $(BUILD)/libteep/ree/libwebsockets/lib/lib* $(prefix)/usr/lib
	rm -f $(prefix)/root/env.sh
	echo 'PATH=$$PATH:/root/teep-device' >>$(prefix)/root/env.sh
	echo 'export TAM_HOST=tamproto_tam_api_1' >>$(prefix)/root/env.sh
	echo 'export TAM_PORT=8888' >>$(prefix)/root/env.sh
	echo 'insmod keystone-driver.ko' >>$(prefix)/root/env.sh
	for f in dota dsta dta iota ista ita; do \
		install $(SOURCE)/scripts/$$f.sh $(prefix)/root/; \
		sed -i 's/127.0.0.1/$$TAM_HOST/' $(prefix)/root/$$f.sh; \
		sed -i 's/3000/$$TAM_PORT/' $(prefix)/root/$$f.sh; \
	done

image: install
	make -C $(KEYSTONE_DIR)/build image

qemu: image
	$(MAKE) run-qemu

run-qemu:
	qemu-system-riscv64 \
		-m 4G \
		-bios $(KEYSTONE_DIR)/build/bootrom.build/bootrom.bin \
		-nographic \
		-machine virt \
		-kernel $(KEYSTONE_DIR)/build/sm.build/platform/generic/firmware/fw_payload.elf \
		-append "console=ttyS0 ro root=/dev/vda cma=256M@0x00000000C0000000" \
		-device virtio-blk-device,drive=hd0 -drive file=$(KEYSTONE_DIR)/build/buildroot.build/images/rootfs.ext2,format=raw,id=hd0 \
		-netdev user,id=net0,net=192.168.100.1/24,dhcpstart=192.168.100.128,hostfwd=tcp::10032-:22 \
		-device virtio-net-device,netdev=net0 \
		-device virtio-rng-pci

TAM_URL ?= http://tamproto_tam_api_1:8888

test: upload-hello-tc-download
	TAM_URL=$(TAM_URL) expect ./script/test.expect
