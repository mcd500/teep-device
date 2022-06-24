TOPDIR = $(CURDIR)/..
include $(TOPDIR)/conf.mk


export TA_CFLAGS = $(TEE_CFLAGS) \
	-g \
	$(SGX_CFLAGS) $(DEBUG_FLAGS) \
	$(addprefix -I, $(INCLUDE_DIRS)) \
	-DCRYPTLIB=$(CRYPTLIB) \
	-I/opt/intel/sgxsdk/include/tlibc \
	-I$(TAREF_DIR) \
	-I$(TAREF_DIR)/gp/include \
	-I$(TAREF_DIR)/api/include \
	-I$(BUILD)/tee/QCBOR/inc \
	-I$(TOPDIR)/include \
	-I$(TOPDIR)/lib/include \
	-DPLAT_SGX \

TA_LIBS = \
	-ltee_api \
	-lmbedtls \
	-lgp \
	-lEnclave_t \
	-lteep \
	-lqcbor \
	-lteesuit \
	-lteecbor \
	-lsgx_tstdc -lsgx_tcxx -lsgx_tcrypto -lsgx_tservice$(LIBRARY_SUFFIX)

export TA_LDFLAGS = \
	-L$(TAREF_DIR)/build/lib \
	-L$(BUILD)/tee/QCBOR \
	-L$(BUILD)/lib \
	-Wl,-z,relro,-z,now,-z,noexecstack \
	-Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L$(SGX_LIBRARY_DIR) -L$(TAREF_DIR)/build/lib \
	-Wl,--whole-archive -lsgx_trts$(LIBRARY_SUFFIX) -Wl,--no-whole-archive \
	-Wl,--start-group $(TA_LIBS) -Wl,--end-group \
	-Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
	-Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
	-Wl,--defsym,__ImageBase=0 -Wl,--gc-sections   \
	-Wl,--version-script=$(TOPDIR)/platform/sgx/Enclave.lds

CFLAGS += $(TA_CFLAGS)

LDFLAGS = $(TA_LDFLAGS)

out-dir ?= .

.PHONY: all
all: $(out-dir)/teep-agent-ta.signed.so

$(out-dir)/%.o: %.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $< -o $@

$(out-dir)/teep-agent-ta.o: teep-agent-ta.c teep-agent-ta.h ta-store.h
$(out-dir)/ta-store.o: ta-store.c teep-agent-ta.h ta-store.h

$(out-dir)/teep-agent-ta.so: $(out-dir)/teep-agent-ta.o $(out-dir)/ta-store.o
	$(CROSS_COMPILE)gcc -nostdlib -o $@ -static $^ $(LDFLAGS)

$(out-dir)/teep-agent-ta.signed.so: $(out-dir)/teep-agent-ta.so
	$(ENCLAVE_SIGNER_BIN) sign \
		-key $(ENCLAVE_PEM) \
		-enclave $< \
		-out $@ \
		-config $(ENCLAVE_CONFIG_FILE)
