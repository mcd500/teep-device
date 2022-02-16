export SOURCE ?= $(CURDIR)/..
export KEYSTONE_SDK_DIR ?= $(KEYSTONE_DIR)/sdk/build64

ifeq ($(TAREF_DIR),)
$(error "Error: Environment variable TAREF_DIR must be set to ta-ref directory")
endif


export APP_CFLAGS = \
	-Wall \
	-I$(KEYSTONE_SDK_DIR)/lib/host/include \
	-I$(KEYSTONE_SDK_DIR)/lib/edge/include \
	-I$(KEYSTONE_SDK_DIR)/lib/verifier \
	-I$(TAREF_DIR)/build/include \
	-I$(TAREF_DIR)/include \
	-I$(SOURCE) \
	-I$(TAREF_DIR)/ref-ta/profiler

export APP_LDFLAGS = \
	-L$(KEYSTONE_SDK_DIR)/lib \
	-L$(KEYSTONE_SDK_DIR)/sdk/lib \
	-L$(TAREF_DIR)/build/lib

export APP_LIBS = \
	-lkeystone-host -lkeystone-edge -lEnclave_u -lflatccrt

export TA_CFLAGS = \
	-Wall -fno-builtin-printf -DEDGE_IGNORE_EGDE_RESULT -DCRYPTLIB=MBEDCRYPT \
	-I$(TAREF_DIR)/gp/include \
	-I$(TAREF_DIR)/api/include \
	-I$(TAREF_DIR)/api/keystone \
	-I$(TAREF_DIR)/build/include \
	-I$(TAREF_DIR)/keyedge/target/include \
	-I$(KEYSTONE_SDK_DIR)/include/app \
	-I$(KEYSTONE_SDK_DIR)/include/edge \
	-I$(SOURCE)

export TA_LDFLAGS = \
	-L$(TAREF_DIR)/build/lib \
	-L$(KEYSTONE_SDK_DIR)/lib \
	-T $(TAREF_DIR)/api/keystone/Enclave.lds

export TA_LIBS = \
	-lgp -lbench \
	-lEnclave_t \
	-lkeystone-eapp \
	-ltee_api -lmbedtls -ltiny_AES_c -ltiny_sha3 -led25519

SHIP_BINS = \
	App-keystone \
	Enclave \
	$(KEYSTONE_SDK_DIR)/runtime/eyrie-rt

QEMU_INSTALLED_BINS = \
	$(prefix)/root/App-keystone \
	$(prefix)/root/Enclave.eapp_riscv \
	$(prefix)/root/eyrie-rt \
	$(prefix)/root/keystone-driver.ko \
	$(prefix)/root/env.sh

export CROSS_COMPILE = riscv64-unknown-linux-gnu-

.PHONY: all
all: App-keystone Enclave

App-keystone.o: $(SOURCE)/App-keystone.cpp
	$(CROSS_COMPILE)g++ -c -o $@ $< $(APP_CFLAGS)

App-keystone: App-keystone.o
	$(CROSS_COMPILE)g++ -o $@ $^ $(APP_LDFLAGS) $(APP_LIBS)

Enclave.o: $(SOURCE)/Enclave.c
	$(CROSS_COMPILE)gcc -c -o $@ $< $(TA_CFLAGS)

Enclave: Enclave.o
	$(CROSS_COMPILE)ld -o $@ $^ $(TA_LDFLAGS) --start-group $(TA_LIBS) --end-group

prefix ?= $(KEYSTONE_DIR)/build/overlay

.PHONY: install-qemu-image
install-qemu-image $(QEMU_INSTALLED_BINS): $(SHIP_BINS)
	test -n "$(prefix)"
	install App-keystone $(prefix)/root/
	install Enclave $(prefix)/root/Enclave.eapp_riscv
	install $(KEYSTONE_SDK_DIR)/runtime/eyrie-rt $(prefix)/root/
	rm -f $(prefix)/root/env.sh
	echo 'PATH=$$PATH:/root/' >>$(prefix)/root/env.sh
	echo 'insmod keystone-driver.ko' >>$(prefix)/root/env.sh
	make -C $(KEYSTONE_DIR)/build image


.PHONY: run-qemu
run-qemu: $(QEMU_INSTALLED_BINS)
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

.PHONY: clean
clean:
	rm -rf *.o App-keystone Enclave
