#global-incdirs-y += libwebsockets/build libwebsockets/lib boringssl/include .
srcs-y += ta-aist-test.c

#BINARY:="8d82573a-926d-4754-9353-32dc29997f74"

CFLAGS+=-nostdinc
LDFLAGS+=-L$(TA_DEV_KIT_DIR)/lib  --whole-archive -lwebsockets-ta --no-whole-archive

# Eliminate unused symbols:
# # 1. Tell compiler to place each data/function item in its own section
#cflags-lib-y += -fdata-sections -ffunction-sections
# # 2. Tell lib.mk to use $(LD) rather than $(AR)
# lib-use-ld := y
# # 3. Tell $(LD) to perform incremental link, keeping symbol 'crypto_ops' and
# # its dependencies, then discarding unreferenced symbols
# lib-ldflags := -i --gc-sections -u crypto_ops
#
# incdirs-lib-y := ../../include
# incdirs-y := include

# To remove a certain compiler flag, add a line like this
#cflags-template_ta.c-y += -Wno-strict-prototypes
