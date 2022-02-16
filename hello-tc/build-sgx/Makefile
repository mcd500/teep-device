ifeq ($(TAREF_DIR),)
$(error "Error: Environment variable TAREF_DIR must be set to ta-ref directory")
endif

export MACHINE ?= SIM

.PHONY: build app enclave
build: app enclave

app:
	make -f app.mk

enclave:
	make -f enclave.mk

clean:
	rm -f *.o App_sgx *.so
