CC = riscv64-unknown-linux-gnu-gcc
CFLAGS = -Wall -fno-builtin-printf -DEDGE_IGNORE_EGDE_RESULT -DCRYPTLIB=MBEDCRYPT
LINK = riscv64-unknown-linux-gnu-gcc
AS = riscv64-unknown-linux-gnu-as

CFLAGS += \
	-I. \
	-I$(TEE_REF_TA_DIR)/api/include \
	-I$(TEE_REF_TA_DIR)/api/keystone \
	-I$(TEE_REF_TA_DIR)/build/include \
	-I$(TEE_REF_TA_DIR)/keyedge/target/include \
	-I$(KEYSTONE_SDK_DIR)/lib/app/include \
	-I$(KEYSTONE_SDK_DIR)/lib/edge/include \
	-I../platform/keystone/build/libteep/tee/libwebsockets/include \
	-DKEYSTONE \
	-DPLAT_KEYSTONE

CFLAGS += -I../libteep/mbedtls/include # XXX: use ta-ref/crypto/include

out-dir ?= .

all: $(out-dir)/teep-agent-ta

$(out-dir)/%.o: %.c
	mkdir -p $(out-dir)
	$(CC) $(CFLAGS) -c $< -o $@

$(out-dir)/teep-agent-ta.o: teep-agent-ta.c
$(out-dir)/teep_message.o: teep_message.c
$(out-dir)/ta-store.o: ta-store.c

LDFLAGS = \
	-L$(TEE_REF_TA_DIR)/build/lib \
	-L$(KEYSTONE_SDK_DIR)/lib \
	-L$(BUILD)/libteep/tee/libwebsockets/lib

LIBS = \
	-ltee_api \
	-lwebsockets \
	-lmbedtls \
	-lEnclave_t \
	-lflatccrt \
	-lkeystone-eapp \
	-lgcc

$(out-dir)/teep-agent-ta: $(out-dir)/teep-agent-ta.o $(out-dir)/teep_message.o $(out-dir)/ta-store.o $(out-dir)/vsnprintf.o $(out-dir)/tools.o
	$(LINK) -nostdlib -o $@ -static $^ $(LDFLAGS) $(LIBS) -T Enclave.lds


