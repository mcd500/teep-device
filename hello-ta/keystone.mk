CC = riscv64-unknown-linux-gnu-gcc
CFLAGS = -Wall -fno-builtin-printf -DEDGE_IGNORE_EGDE_RESULT -DCRYPTLIB=MBEDCRYPT
LINK = riscv64-unknown-linux-gnu-ld
AS = riscv64-unknown-linux-gnu-as

CFLAGS += \
	-I. \
	-I$(TEE_REF_TA_DIR)/build/include/api \
	-I$(TEE_REF_TA_DIR)/build/include \
	-I$(TEE_REF_TA_DIR)/keyedge/target/include \
	-I$(KEYSTONE_SDK_DIR)/lib/app/include \
	-I$(KEYSTONE_SDK_DIR)/lib/edge/include \
	-DKEYSTONE

out-dir ?= .

all: $(out-dir)/hello-ta

$(out-dir)/hello-ta.o: hello-ta.c
	mkdir -p $(out-dir)
	$(CC) $(CFLAGS) -c $< -o $@

LDFLAGS = \
	-L$(TEE_REF_TA_DIR)/build/lib \
	-L$(KEYSTONE_SDK_DIR)/lib

LIBS = \
	-lEnclave_t \
	-lflatccrt \
	-lkeystone-eapp \
	-ltee_api

$(out-dir)/hello-ta: $(out-dir)/hello-ta.o
	$(LINK) -o $@ -static $(out-dir)/hello-ta.o $(LDFLAGS) $(LIBS) -T Enclave.lds

