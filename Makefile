.PHONY: all all- all-optee all-keystone
.PHONY: clean clean-optee clean-keystone
.PHONY: test test-optee test-keystone
.PHONY: qemu qemu-optee qemu-keystone

all: all-$(TEE)

clean: clean-optee clean-keystone

test: test-$(TEE)

qemu: qemu-$(TEE)

.PHONY: distclean
distclean: clean
	rm -fr sample-senario/node_modules/ sample-senario/package-lock.json
	rm -fr test-jw
	rm -f $(TEEP_KEY_SRCS)



all-:
	@echo '$$TEE must be "optee" or "keystone"'
	@false

all-optee: build-optee

all-keystone: build-keystone

clean-optee:
	$(MAKE) -C platform/op-tee clean

clean-keystone:
	$(MAKE) -C platform/keystone clean

.PHONY: build-optee build-keystone

build-optee:
	$(MAKE) -C platform/op-tee install_qemu

build-keystone:
	$(MAKE) -C platform/keystone image

test-optee:
	$(MAKE) -C platform/op-tee test

test-keystone:
	$(MAKE) -C platform/keystone test

qemu-optee:
	$(MAKE) -C platform/op-tee run-qemu

qemu-keystone:
	$(MAKE) -C platform/keystone run-qemu
