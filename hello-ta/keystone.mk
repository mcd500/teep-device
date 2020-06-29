ifndef KEYSTONE_SDK_DIR
export KEYSTONE_SDK_DIR = $(KEYSTONE_DIR)/sdk
endif

CC = riscv64-unknown-linux-gnu-gcc
CFLAGS = -Wall -fno-builtin-printf -DEDGE_IGNORE_EGDE_RESULT -DCRYPTLIB=MBEDCRYPT
LINK = riscv64-unknown-linux-gnu-ld
AS = riscv64-unknown-linux-gnu-as

TEE_REF_TA_DIR = $(CURDIR)/../..

CFLAGS += -I. -I$(TEE_REF_TA_DIR)/include -I$(TEE_REF_TA_DIR)/platform/keystone -I$(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/ -I$(TEE_REF_TA_DIR)/keyedge/target/include -I$(TEE_REF_TA_DIR)/ref-ta/keystone/Edge -I$(KEYSTONE_SDK_DIR)/lib/app/include  -I$(KEYSTONE_SDK_DIR)/lib/edge/include

all: hello-ta.keystone

hello-ta.keystone.o: hello-ta.c
	$(CC) $(CFLAGS) -c $< -o $@

OBJ = \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/Enclave_t.o \
  $(TEE_REF_TA_DIR)/platform/keystone/tee-internal-api-keystone.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/../libbuild/ed25519/sign.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/../libbuild/ed25519/keypair.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/../libbuild/ed25519/seed.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/../libbuild/ed25519/sha512.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/../libbuild/ed25519/add_scalar.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/../libbuild/ed25519/ge.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/../libbuild/ed25519/fe.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/../libbuild/ed25519/key_exchange.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/../libbuild/ed25519/sc.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/../libbuild/ed25519/verify.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/../libbuild/cryptlib/cipher.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/../libbuild/cryptlib/platform_util.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/../libbuild/cryptlib/aes.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/../libbuild/cryptlib/gcm.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/../libbuild/cryptlib/cipher_wrap.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/../libbuild/tiny-AES-c/aes.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/../libbuild/tiny_sha3/sha3.o \
  $(TEE_REF_TA_DIR)/build-keystone/sdk/lib/libkeystone-eapp.a \
  $(TEE_REF_TA_DIR)/build-keystone/sdk/lib/libkeystone-edge.a \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/../../keyedge/lib/flatccrt.a

hello-ta.keystone: hello-ta.keystone.o ref-ta
	$(LINK) -o $@ -static hello-ta.keystone.o $(OBJ) -T $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/Enclave.lds

.PHONY: ref-ta
ref-ta:
	-make -C $(TEE_REF_TA_DIR)/ref-ta/keystone
