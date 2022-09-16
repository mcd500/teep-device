TOPDIR = $(CURDIR)
include $(TOPDIR)/conf.mk

.PHONY: all
all: check-tee submodule suit libteep agent broker hello-tc rootfs

.PHONY: clean
clean: clean-hello-tc clean-docs
	rm -rf build

.PHONY: submodule
submodule:
	$(MAKE) -C submodule

suit-FLAGS = \
	-DCMAKE_BUILD_TYPE=DEBUG \
	-DENABLE_LOG_STDOUT=OFF \
	-DENABLE_EXAMPLE=OFF

.PHONY: suit
suit:
	mkdir -p build/$(TEE)/tee/suit
	cd build/$(TEE)/tee/suit && cmake $(TOPDIR)/suit $(suit-FLAGS) $(cmake-tee-TOOLCHAIN)
	$(MAKE) -C build/$(TEE)/tee/suit

.PHONY: libteep
libteep:
	$(MAKE) -C libteep/lib

.PHONY: agent
agent:
	$(MAKE) -C teep-agent-ta -f $(TEE).mk out-dir=$(BUILD)/agent

.PHONY: broker
broker:
	$(MAKE) -C teep-broker-app

.PHONY: hello-tc
hello-tc:
	$(MAKE) -C hello-tc/build-$(PLAT) SOURCE=$(TOPDIR)/hello-tc

.PHONY: clean-hello-tc
clean-hello-tc:
	$(MAKE) -C hello-tc/build-keystone SOURCE=$(TOPDIR)/hello-tc clean
	$(MAKE) -C hello-tc/build-optee SOURCE=$(TOPDIR)/hello-tc clean
	$(MAKE) -C hello-tc/build-sgx SOURCE=$(TOPDIR)/hello-tc clean
	$(MAKE) -C hello-tc/build-pc SOURCE=$(TOPDIR)/hello-tc clean

.PHONY: rootfs
rootfs:
	$(MAKE) -C sample rootfs

.PHONY: docs
docs:
	@echo "Generating doxygen files"
	@doxygen docs/doxygen/Doxyfile
	make -C docs/doxygen/latex
	cp docs/doxygen/latex/refman.pdf docs/teep-device.pdf	
	rm -fr docs/teep-device_readme_html
	mv docs/doxygen/html docs/teep-device_readme_html
	cd docs; tar czf teep-device_readme_html.tar.gz open-readme.html teep-device_readme_html

gen_readme:
	cat docs/overview_of_teep-device.md docs/building_with_docker.md \
		docs/building_pc.md > README.md
	sed -i 's/@image html /![](/g' README.md
	sed -i '/^\!\[\]/ s/$$/)/' README.md
	sed -i '/^@image latex/d' README.md

clean-docs:
	rm -f -r docs/doxygen/html
	rm -f -r docs/doxygen/latex
	rm -f -r docs/teep-device_readme_html
	rm -f -r docs/teep-device_readme_html.tar.gz
	rm -f -r docs/teep-device.pdf

.PHONY: run-sample-session
run-sample-session: check-tee
	$(MAKE) -C sample run-session

.PHONY: run-qemu
run-qemu: check-tee
	$(MAKE) -C sample run-qemu

.PHONY: check-tee
ifeq ($(TEE),)
check-tee:
	@echo '$$TEE must be "optee", "keystone", "sgx" or "pc"'
	@false
else
check-tee:
endif

