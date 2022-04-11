export PLAT = keystone
export CROSS_COMPILE = riscv64-unknown-linux-gnu-
export KEYSTONE_SDK_DIR ?= $(KEYSTONE_DIR)/sdk

export TEE_CFLAGS = \
	-I$(TAREF_DIR)/build/include \
	-I$(TEE_REF_TA_DIR)/include \
	-I$(KEYSTONE_SDK_DIR)/include/app \
	-I$(KEYSTONE_SDK_DIR)/include/edge

export TEE_LDFLAGS = \
	-L$(TAREF_DIR)/build/lib \
	-L$(KEYSTONE_SDK_DIR)/lib \
	-T $(TAREF_DIR)/api/keystone/Enclave.lds

export TEE_LIBS = \
	-ltee_api \
	-lEnclave_t \
	-lflatccrt \
	-lkeystone-eapp

export REE_CFLAGS = \
	-I$(TAREF_DIR)/build/include \
	-I$(KEYSTONE_SDK_DIR)/lib/host/include \
	-I$(KEYSTONE_SDK_DIR)/lib/edge/include \
	-DPLAT_KEYSTONE

export REE_LDFLAGS = \
	-L$(KEYSTONE_SDK_DIR)/lib \
	-L$(TAREF_DIR)/build/lib

export REE_LIBS = \
	-lkeystone-host -lkeystone-edge -lEnclave_u -lflatccrt

TOOLCHAIN-ree = $(TOPDIR)/platform/keystone/cross-riscv64.cmake
TOOLCHAIN-tee = $(TOPDIR)/platform/keystone/cross-riscv64.cmake

cmake-ree-TOOLCHAIN = -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN-ree)
cmake-tee-TOOLCHAIN = -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN-tee)
