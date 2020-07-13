all: build-optee build-keystone

.PHONY: clean
clean: clean-optee clean-keystone

.PHONY: update
update:
	scp libteep.so root@${HIKEY_IP}:/usr/bin

build-optee:
	$(MAKE) -C ../platform/op-tee

build-keystone:
	$(MAKE) -C ../platform/keystone

clean-optee:
	$(MAKE) -C ../platform/op-tee clean

clean-keystone:
	$(MAKE) -C ../platform/keystone clean

.PHONY: all clean update
.PHONY: build-optee build-keystone
.PHONY: clean-optee clean-keystone
