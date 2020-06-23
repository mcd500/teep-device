ifndef KEYSTONE_SDK_DIR
export KEYSTONE_SDK_DIR = $(KEYSTONE_DIR)/sdk
endif

CC = riscv64-unknown-linux-gnu-gcc
CFLAGS = -Wall -fno-builtin-printf -DEDGE_IGNORE_EGDE_RESULT
LINK = riscv64-unknown-linux-gnu-ld
AS = riscv64-unknown-linux-gnu-as

TEE_REF_TA_DIR = $(CURDIR)/../..

CFLAGS += -I. -I$(TEE_REF_TA_DIR)/include

all: hello-ta.keystone.o

hello-ta.keystone.o: hello-ta.c
	$(CC) $(CFLAGS) -c $< -o $@
