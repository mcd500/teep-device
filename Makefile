.PHONY: all all- all-optee all-keystone all-pc docs
.PHONY: clean clean-optee clean-keystone clean-pc clean-docs
.PHONY: test test-optee test-keystone test-pc
.PHONY: qemu qemu-optee qemu-keystone

all: $(KEY_HEADERS) all-$(TEE)

KEYS := $(CURDIR)/key/test-jw_tee_identity_private_tee-mytee-private.jwk \
	$(CURDIR)/key/test-jw_tee_identity_tee-mytee-public.jwk \
	$(CURDIR)/key/test-jw_tee_sds_xbank_spaik-priv.jwk \
	$(CURDIR)/key/test-jw_tee_sds_xbank_spaik-pub.jwk \
	$(CURDIR)/key/test-jw_tsm_identity_private_tam-mytam-private.jwk \
	$(CURDIR)/key/test-jw_tsm_identity_tam-mytam-public.jwk

KEY_HEADERS := $(CURDIR)/key/include/sp_pubkey_jwk.h \
	$(CURDIR)/key/include/tam_id_pubkey_jwk.h \
	$(CURDIR)/key/include/tee_id_privkey_jwk.h \
	$(CURDIR)/key/include/tee_id_pubkey_jwk.h

.PHONY: gen-keys
gen-keys $(KEYS):
	scripts/keygen/genkeys.sh

.PHONY: gen-key-headers
gen-key-headers $(KEY_HEADERS): $(KEYS)
	scripts/keygen/genheaders.sh

clean: clean-optee clean-keystone clean-pc clean-docs

test: test-$(TEE)

qemu: qemu-$(TEE)

.PHONY: distclean
distclean: clean
	rm -fr sample-senario/node_modules/ sample-senario/package-lock.json
	rm -fr test-jw
	rm -f $(TEEP_KEY_SRCS)



all-:
	@echo '$$TEE must be "optee", "keystone" or "pc"'
	@false

all-optee: build-optee

all-keystone: build-keystone

all-pc: build-pc

docs:
	@echo "Generating doxygen files"
	@doxygen docs/doxygen/Doxyfile
	make -C docs/doxygen/latex
	cp docs/doxygen/latex/refman.pdf docs/teep-device.pdf	
	rm -fr docs/teep-device_readme_html
	mv docs/doxygen/html docs/teep-device_readme_html
	cd docs; tar czf teep-device_readme_html.tar.gz open-readme.html teep-device_readme_html


clean-docs:
	rm -f -r docs/doxygen/html
	rm -f -r docs/doxygen/latex
	rm -f -r docs/teep-device_readme_html
	rm -f -r docs/teep-device_readme_html.tar.gz
	rm -f -r docs/teep-device.pdf

clean-optee:
	$(MAKE) -C platform/op-tee clean

clean-keystone:
	$(MAKE) -C platform/keystone clean

clean-pc:
	$(MAKE) -C platform/pc clean

.PHONY: build-optee build-keystone

build-optee:
	$(MAKE) -C platform/op-tee

optee_install_qemu:
	$(MAKE) -C platform/op-tee install_qemu

build-keystone:
	$(MAKE) -C platform/keystone image

build-pc:
	$(MAKE) -C platform/pc

build-keystone-trvsim:
	$(MAKE) -C platform/keystone ship-trvsim PORT=$(TRVSIM_PORT)


test-optee:
	$(MAKE) -C platform/op-tee test

test-keystone:
	$(MAKE) -C platform/keystone test

test-pc:
	$(MAKE) -C platform/pc test

test-keystone-trvsim:
	$(MAKE) -C platform/keystone test-trvsim PORT=$(TRVSIM_PORT)

qemu-optee:
	$(MAKE) -C platform/op-tee run-qemu

qemu-keystone:
	$(MAKE) -C platform/keystone run-qemu
