.PHONY: all all- all-optee all-keystone all-pc
.PHONY: clean clean-optee clean-keystone clean-pc
.PHONY: test test-optee test-keystone test-pc
.PHONY: qemu qemu-optee qemu-keystone

all: all-$(TEE)

clean: clean-optee clean-keystone clean-pc

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

clean-optee:
	$(MAKE) -C platform/op-tee clean

clean-keystone:
	$(MAKE) -C platform/keystone clean

clean-pc:
	$(MAKE) -C platform/pc clean

.PHONY: build-optee build-keystone

build-optee:
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
