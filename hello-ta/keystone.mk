ifndef KEYSTONE_SDK_DIR
export KEYSTONE_SDK_DIR = $(KEYSTONE_DIR)/sdk
endif

CC = riscv64-unknown-linux-gnu-gcc
CFLAGS = -Wall -fno-builtin-printf -DEDGE_IGNORE_EGDE_RESULT
LINK = riscv64-unknown-linux-gnu-ld
AS = riscv64-unknown-linux-gnu-as

TEE_REF_TA_DIR = $(CURDIR)/../..

CFLAGS += -I. -I$(TEE_REF_TA_DIR)/include

all: hello-ta.keystone

hello-ta.keystone.o: hello-ta.c
	$(CC) $(CFLAGS) -c $< -o $@

hello-ta.keystone: hello-ta.keystone.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/secure_storage.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/symmetric_key_gcm.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/time.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/asymmetric_key.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/symmetric_key.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/message_digest.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/tee_wrapper.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/random.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/printf.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/tools.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/strcmp.o \
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/startup.o \
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
  $(TEE_REF_TA_DIR)/ref-ta/keystone/Enclave/../../profiler/libprofiler.a
	$(LINK) -o $@ -static $^

