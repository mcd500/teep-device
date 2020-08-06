export BUILD ?= $(CURDIR)/build
export SOURCE ?= $(CURDIR)/../..
export PLAT = keystone

export KEYSTONE_SDK_DIR ?= $(KEYSTONE_DIR)/sdk

#export HELLO_TA_UUID  ?= 8d82573a-926d-4754-9353-32dc29997f74
#export TEE_AGENT_UUID ?= 68373894-5bb3-403c-9eec-3114a1f5d3fc

export TEE_REF_TA_DIR = $(CURDIR)/../../..

export APP_CFLAGS = \
	-I$(KEYSTONE_SDK_DIR)/lib/host/include \
	-I$(KEYSTONE_SDK_DIR)/lib/edge/include \
	-I$(KEYSTONE_SDK_DIR)/lib/verifier \
	-I$(TEE_REF_TA_DIR)/build/include \
	-I./ \
	-I$(TEE_REF_TA_DIR)/ref-ta/profiler \
	-I$(SOURCE)/libteep/lib \
	-I$(SOURCE)/platform/keystone/build/libteep/ree/libwebsockets/include \
	-I$(SOURCE)/libteep/libwebsockets/include \
	-I$(SOURCE)/libteep/mbedtls/include \
	-DKEYSTONE -DAPP_VERBOSE -Wall

export APP_LDFLAGS = \
	-L$(KEYSTONE_SDK_DIR)/lib \
	-L$(KEYSTONE_SDK_DIR)/sdk/lib \
	-L$(TEE_REF_TA_DIR)/build/lib \
	-L$(SOURCE)/platform/keystone/build/libteep/ree/mbedtls/library \
	-Xlinker -rpath-link -Xlinker $(SOURCE)/platform/keystone/build/libteep/ree/mbedtls/library \
	-L$(SOURCE)/platform/keystone/build/libteep/ree/libwebsockets/lib \
	-L$(SOURCE)/platform/keystone/build/libteep/ree/lib

export APP_LIBS = \
	-lkeystone-host -lkeystone-edge -lEnclave_u -lflatccrt -lmbedcrypto -lmbedx509 -lmbedtls -lwebsockets -lteep \
	-ltee_api

TOOLCHAIN-ree = $(CURDIR)/cross-riscv64.cmake
TOOLCHAIN-tee = $(CURDIR)/cross-riscv64.cmake

REE_CFLAGS = -Wall -Werror -fPIC \
	      -I$(BUILD)/libteep/ree/libwebsockets/include \
		  -I$(SOURCE)/libteep/libwebsockets/include \
	      -I$(SOURCE)/libteep/mbedtls/include $(INCLUDES) \
	      -I$(TEE_REF_TA_DIR)/build/include \
	      -DPLAT_KEYSTONE

REE_LDFLAGS = \
	-L$(BUILD)/libteep/ree/libwebsockets/lib \
	-L$(BUILD)/libteep/ree/mbedtls/library \
	$(TEE_REF_TA_DIR)/api/libtee_api.a
export CROSS_COMPILE = riscv64-unknown-linux-gnu-
LIBTEEP_SRC = $(SOURCE)/libteep/lib/main.c

libteep-mbedtls-host-DISABLE = y
libteep-libwebsockets-host-DISABLE = y

#libteep-mbedtls-tee-DISABLE = y

libteep-mbedtls-ree-FLAGS = \
	-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN-ree) \
	-DCMAKE_INSTALL_PREFIX:PATH=staging \
	-DENABLE_TESTING=0  \
	-DCMAKE_BUILD_TYPE=RELEASE \
	-DUSE_SHARED_MBEDTLS_LIBRARY=1

libteep-libwebsockets-ree-FLAGS = \
	-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN-ree) \
	-DLWS_ROLE_WS=0 \
	-DLWS_WITH_SSL=1 \
	-DLWS_WITH_MBEDTLS=1 \
	-DMBEDTLS_LIBRARY=$(BUILD)/libteep/ree/mbedtls/library/libmbedtls.so \
	-DMBEDX509_LIBRARY=$(BUILD)/libteep/ree/mbedtls/library/libmbedx509.so \
	-DMBEDCRYPTO_LIBRARY=$(BUILD)/libteep/ree/mbedtls/library/libmbedcrypto.so \
	-DLWS_MBEDTLS_INCLUDE_DIRS="$(SOURCE)/libteep/mbedtls/include" \
	-DLWS_WITH_JOSE=1 \
	-DLWS_WITHOUT_SERVER=1 \
	-DLWS_WITH_STATIC=0 \
	-DLWS_WITH_SHARED=1 \
	-DLWS_STATIC_PIC=1 \
	-DLWS_MAX_SMP=1 \
	-DCMAKE_BUILD_TYPE=RELEASE \
	-DLWS_WITHOUT_TESTAPPS=1 \
	-DLWS_WITHOUT_EXTENSIONS=1 \
	-DLWS_WITH_ZLIB=0

libteep-mbedtls-tee-FLAGS = \
	-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN-tee) \
	-DCMAKE_INSTALL_PREFIX:PATH=staging \
	-DENABLE_TESTING=0  \
	-DCMAKE_BUILD_TYPE=RELEASE \
	-DUSE_SHARED_MBEDTLS_LIBRARY=1

libteep-libwebsockets-tee-FLAGS = \
	-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN-tee) \
	-DLWS_PLAT_OPTEE=1 \
	-DLWS_PLAT_KEYSTONE=1 \
	-DLWS_ROLE_WS=0 \
	-DLWS_WITH_SSL=1 \
	-DLWS_WITH_MBEDTLS=1 \
	-DMBEDTLS_LIBRARY=$(BUILD)/libteep/tee/mbedtls/library/libmbedtls.a \
	-DMBEDX509_LIBRARY=$(BUILD)/libteep/tee/mbedtls/library/libmbedx509.a \
	-DMBEDCRYPTO_LIBRARY=$(BUILD)/libteep/tee/mbedtls/library/libmbedcrypto.a \
	-DLWS_MBEDTLS_INCLUDE_DIRS="$(SOURCE)/libteep/mbedtls/include" \
	-DLWS_WITH_JOSE=1 \
	-DLWS_WITHOUT_SERVER=1 \
	-DLWS_WITH_STATIC=1 \
	-DLWS_WITH_SHARED=0 \
	-DLWS_STATIC_PIC=1 \
	-DLWS_MAX_SMP=1 \
	-DCMAKE_BUILD_TYPE=RELEASE \
	-DLWS_WITHOUT_TESTAPPS=1 \
	-DLWS_WITHOUT_EXTENSIONS=1 \
	-DLWS_WITH_ZLIB=0 \
	-DLWS_WITH_DIR=0 \
	-DLWS__INCLUDE_DIRS=$(INCLUDES) \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_C_FLAGS="-Wno-unused-variable -I$(TEE_REF_TA_DIR)/build/include"

all: libteep hello teep

include ../common.mk

clean:
	rm -rf $(BUILD)


prefix ?= $(KEYSTONE_DIR)/buildroot_overlay

install: all
	test -n "$(prefix)"
	mkdir -p $(prefix)/root/teep-device
	install $(BUILD)/hello-ta/hello-ta $(prefix)/root/teep-device
	install $(BUILD)/hello-app/hello-app $(prefix)/root/teep-device
	install $(BUILD)/teep-agent-ta/teep-agent-ta $(prefix)/root/teep-device
	install $(BUILD)/teep-broker-app/teep-broker-app $(prefix)/root/teep-device
	install $(KEYSTONE_DIR)/sdk/rts/eyrie/eyrie-rt $(prefix)/root/teep-device
	mkdir -p $(prefix)/usr/lib
	install $(BUILD)/libteep/ree/mbedtls/library/lib* $(prefix)/usr/lib
	install $(BUILD)/libteep/ree/libwebsockets/lib/lib* $(prefix)/usr/lib
	install $(BUILD)/libteep/ree/lib/lib* $(prefix)/usr/lib

image: install
	make -C $(KEYSTONE_DIR) image

qemu: image
	$(MAKE) run-qemu

run-qemu:
	$(KEYSTONE_DIR)/riscv-qemu/riscv64-softmmu/qemu-system-riscv64 \
		-m 4G \
		-bios $(KEYSTONE_DIR)/bootrom/bootrom.elf \
		-nographic \
		-machine virt \
		-kernel $(KEYSTONE_DIR)/hifive-work/riscv-pk/bbl \
		-netdev user,id=net0,net=192.168.100.1/24,dhcpstart=192.168.100.128,hostfwd=tcp::10032-:22 \
		-device virtio-net-device,netdev=net0

test: image
	-ssh-keygen -R '[localhost]:10032'
	screen -L -dmS qemu make run-qemu
	sleep 7
	PORT=10032 USER=root IP_ADDR=localhost PASSWD=sifive ./script/test.expect
	screen -S qemu -X poweroff
	pkill screen
	pkill qemu
