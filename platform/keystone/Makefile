export BUILD ?= $(CURDIR)/build
export SOURCE ?= $(CURDIR)/../..
export PLAT = keystone

export KEYSTONE_SDK_DIR ?= $(KEYSTONE_DIR)/sdk

export HELLO_TA_UUID  ?= 8d82573a-926d-4754-9353-32dc29997f74
export TEE_AGENT_UUID ?= 68373894-5bb3-403c-9eec-3114a1f5d3fc

export TEE_REF_TA_DIR = $(CURDIR)/../../..

export APP_CFLAGS = \
	-Wall \
	-I$(KEYSTONE_SDK_DIR)/lib/host/include \
	-I$(KEYSTONE_SDK_DIR)/lib/edge/include \
	-I$(KEYSTONE_SDK_DIR)/lib/verifier \
	-I$(TEE_REF_TA_DIR)/build/include \
	-I$(TEE_REF_TA_DIR)/include \
	-I./ \
	-I$(TEE_REF_TA_DIR)/ref-ta/profiler \
	-I$(SOURCE)/libteep/lib \
	-I$(BUILD)/libteep/ree/libwebsockets/include \
	-I$(SOURCE)/libteep/libwebsockets/include \
	-I$(SOURCE)/libteep/mbedtls/include \
	-DKEYSTONE -DPLAT_KEYSTONE -DAPP_VERBOSE -Wall

export APP_LDFLAGS = \
	-L$(KEYSTONE_SDK_DIR)/lib \
	-L$(KEYSTONE_SDK_DIR)/sdk/lib \
	-L$(TEE_REF_TA_DIR)/build/lib \
	-L$(BUILD)/libteep/ree/mbedtls/library \
	-Xlinker -rpath-link -Xlinker $(BUILD)/libteep/ree/mbedtls/library \
	-L$(BUILD)/libteep/ree/libwebsockets/lib \
	-L$(BUILD)/libteep/ree/lib

export APP_LIBS = \
	-lkeystone-host -lkeystone-edge -lEnclave_u -lflatccrt -lmbedcrypto -lmbedx509 -lmbedtls -lwebsockets -lteep \
	-ltee_api

export TA_CFLAGS = \
	-Wall -fno-builtin-printf -DEDGE_IGNORE_EGDE_RESULT -DCRYPTLIB=MBEDCRYPT \
	-I. \
	-I$(TEE_REF_TA_DIR)/api/include \
	-I$(TEE_REF_TA_DIR)/api/keystone \
	-I$(TEE_REF_TA_DIR)/build/include \
	-I$(TEE_REF_TA_DIR)/keyedge/target/include \
	-I$(KEYSTONE_SDK_DIR)/lib/app/include \
	-I$(KEYSTONE_SDK_DIR)/lib/edge/include \
	-I../platform/keystone/build/libteep/tee/libwebsockets/include \
	-I$(SOURCE)/key/include \
	-DKEYSTONE \
	-DPLAT_KEYSTONE

export TA_LDFLAGS = \
	-L$(TEE_REF_TA_DIR)/build/lib \
	-L$(KEYSTONE_SDK_DIR)/lib \
	-L$(BUILD)/libteep/tee/libwebsockets/lib \
	-T $(CURDIR)/Enclave.lds

export TA_LIBS = \
	-lwebsockets \
	-lmbedtls \
	-ltee_api \
	-lEnclave_t \
	-lflatccrt \
	-lkeystone-eapp


TA_CFLAGS += -I../libteep/mbedtls/include # XXX: use ta-ref/crypto/include

TOOLCHAIN-ree = $(CURDIR)/cross-riscv64.cmake
TOOLCHAIN-tee = $(CURDIR)/cross-riscv64.cmake

REE_CFLAGS = -Wall -Werror -fPIC \
	      -I$(BUILD)/libteep/ree/libwebsockets/include \
		  -I$(SOURCE)/libteep/libwebsockets/include \
	      -I$(SOURCE)/libteep/mbedtls/include $(INCLUDES) \
	      -I$(TEE_REF_TA_DIR)/build/include \
	      -I$(TEE_REF_TA_DIR)/include \
	      -DPLAT_KEYSTONE

REE_LDFLAGS = \
	-L$(BUILD)/libteep/ree/libwebsockets/lib \
	-L$(BUILD)/libteep/ree/mbedtls/library \

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
	-DCMAKE_C_FLAGS="-Wno-unused-variable -I$(TEE_REF_TA_DIR)/build/include -I$(TEE_REF_TA_DIR)/include -DCRYPTLIB=MBEDCRYPT -DPLAT_KEYSTONE"

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

SHIP_BINS=$(BUILD)/hello-ta/hello-ta \
	$(BUILD)/hello-app/hello-app \
	$(BUILD)/teep-agent-ta/teep-agent-ta \
	$(BUILD)/teep-broker-app/teep-broker-app \
	$(KEYSTONE_DIR)/sdk/rts/eyrie/eyrie-rt

SHIP_LIBS=$(BUILD)/libteep/ree/mbedtls/library/lib* \
	$(BUILD)/libteep/ree/libwebsockets/lib/lib* \
	$(BUILD)/libteep/ree/lib/lib*

ship-trvsim: all
	sshpass -p sifive ssh -p $(SSH_PORT) -o "StrictHostKeyChecking no" mkdir -p /root/teep-device /usr/lib
	sshpass -p sifive scp -P $(SSH_PORT) -o "StrictHostKeyChecking no" $(SHIP_BINS) root@trvsim:/root/teep-device
	sshpass -p sifive scp -P $(SSH_PORT) -o "StrictHostKeyChecking no" $(SHIP_LIBS) root@trvsim:/usr/lib

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

TAM_URL ?= http://tamproto_tam_api_1:8888

upload_encfile_to_tamproto:
	cd $(SOURCE)/scripts/keygen && npm install
	node $(SOURCE)/scripts/keygen/sign-then-enc.js \
		$(SOURCE)/key/test-jw_tee_sds_xbank_spaik-priv.jwk \
		$(SOURCE)/key/test-jw_tee_identity_tee-mytee-public.jwk \
		$(BUILD)/hello-ta/hello-ta
	curl $(TAM_URL)/api/
	curl $(TAM_URL)/panel/upload \
		-F "file=@$(BUILD)/hello-ta/hello-ta.sign.enc;filename=$(HELLO_TA_UUID).ta"

test-trvsim: upload_encfile_to_tamproto
	TAM_URL=$(TAM_URL) expect ./script/test_trvsim.expect

test-qemu: upload_encfile_to_tamproto
	TAM_URL=$(TAM_URL) expect ./script/test.expect

test: test-pre
	TAM_URL=$(TAM_URL) expect ./script/test.expect
