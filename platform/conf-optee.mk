PLAT = optee
export CROSS_COMPILE = aarch64-linux-gnu-

export OPTEE_DEV_KIT_DIR ?= $(OPTEE_DIR)/optee_os/out/arm
export TA_DEV_KIT_DIR ?= $(OPTEE_DEV_KIT_DIR)/export-ta_arm64

export TEE_CFLAGS = \
	-I$(TAREF_DIR)/include \
	-I$(TAREF_DIR)/build/include

export REE_CFLAGS ?= \
	-DPLAT_OPTEE=1 \
	-I$(OPTEE_DIR)/optee_client/public

export REE_LDFLAGS ?= \
	-L$(OPTEE_DIR)/out-br/target/usr/lib \
	-L$(OPTEE_DIR)/optee_client/out/libteec \
	-L$(TAREF_DIR)/build/lib

export REE_LIBS ?= \
	-lteec

TOOLCHAIN-ree = $(TOPDIR)/platform/op-tee/cross-aarch64.cmake
TOOLCHAIN-tee = $(TOPDIR)/platform/op-tee/cross-aarch64-tee.cmake

cmake-ree-TOOLCHAIN = -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN-ree)
cmake-tee-TOOLCHAIN = -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN-tee)
