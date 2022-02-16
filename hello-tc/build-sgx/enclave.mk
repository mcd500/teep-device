ifeq ($(MACHINE), SIM)
LIBRARY_SUFFIX = _sim
else
LIBRARY_SUFFIX =
endif

CFLAGS = $(SGX_CFLAGS) $(DEBUG_FLAGS)
CFLAGS += $(addprefix -I, $(INCLUDE_DIRS))

CFLAGS += \
	-I$(TAREF_DIR)/gp/include \
	-I$(TAREF_DIR)/api/include \
	-I$(TAREF_DIR)/build/include

#LIBS = tee_api mbedtls tiny_AES_c tiny_sha3 ed25519 wolfssl
#LIBS += gp etools
#LIBS += bench
#LIBS += Enclave_t
# default
#LIBS = sgx_tstdc sgx_tcxx sgx_tcrypto sgx_tservice$(LIBRARY_SUFFIX)

LIBS = \
	-ltee_api \
	-lmbedtls \
	-ltiny_AES_c \
	-ltiny_sha3 \
	-led25519 \
	-lgp \
	-lbench \
	-lEnclave_t \
	-lsgx_tstdc -lsgx_tcxx -lsgx_tcrypto -lsgx_tservice$(LIBRARY_SUFFIX)

# These flags are stereotyped. For more information, see https://github.com/intel/linux-sgx/blob/9ddec08fb98c1636ed3b1a77bbc4fa3520344ede/SampleCode/SampleEnclave/Makefile#L132-L149.
Enclave_Security_Link_Flags := -Wl,-z,relro,-z,now,-z,noexecstack
Enclave_Link_Flags := $(Enclave_Security_Link_Flags) \
    -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L$(SGX_LIBRARY_DIR) -L$(TAREF_DIR)/build/lib \
	-Wl,--whole-archive -lsgx_trts$(LIBRARY_SUFFIX) -Wl,--no-whole-archive \
	-Wl,--start-group $(LIBS) -Wl,--end-group \
	-Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
	-Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
	-Wl,--defsym,__ImageBase=0 -Wl,--gc-sections   \
	-Wl,--version-script=Enclave.lds

LDFLAGS = $(Enclave_Link_Flags)

# input file when signing
ENCLAVE_PEM = Enclave_private.pem
ENCLAVE_CONFIG_FILE = config/Enclave.config.xml

ENCLAVE_LIBRARY = enclave.so
SIGNED_ENCLAVE_LIBRARY = $(ENCLAVE_LIBRARY:.so=.signed.so)
.PHOHY: all clean

all: enclave.signed.so

Enclave.o: ../Enclave.c
	$(CC) $(CFLAGS) -c $< -o $@

enclave.so: Enclave.o
	$(CXX) $^ -o $@ $(LDFLAGS)

enclave.signed.so: enclave.so
	$(ENCLAVE_SIGNER_BIN) sign \
		-key $(ENCLAVE_PEM) \
		-enclave $< \
		-out $@ \
		-config $(ENCLAVE_CONFIG_FILE)

clean:
	$(RM) *.o *.so
