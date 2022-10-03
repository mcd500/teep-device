TOPDIR := $(CURDIR)
export TOPDIR

# Declared $(BUILD) here
include $(TOPDIR)/conf.mk

export DEBUG ?= 1
ifeq ($(DEBUG), 1)
	DFLAGS += '-DDEBUG'
endif

prefix      ?= /usr/local
prefix_bin  ?= teep-broker # Historically it is bin
prefix_lib  ?= $(prefix)/lib

TAM_IP  ?= tamproto_tam_api_1
TAM_URL ?= http://$(TAM_IP):8888


.PHONY: all
all: check-tee submodule lib agent broker hello-tc rootfs

.PHONY: clean
clean: clean-hello-tc clean-docs
	rm -rf build

.PHONY: submodule
submodule:
	$(MAKE) -C submodule

ifeq ($(TEE),pc)
lib-FLAGS += \
	-DCMAKE_BUILD_TYPE=Debug \
	-DTEE_PLATFORM=$(TEE) \
	-DCMAKE_C_FLAGS='$(DFLAGS) -I$(TOPDIR)/submodule/mbedtls/include -I$(BUILD)/tee/QCBOR/inc' \
	-DCMAKE_CXX_FLAGS='$(DFLAGS) -I$(TOPDIR)/submodule/mbedtls/include' \
	-DCMAKE_LIBRARY_PATH="$(BUILD)/ree/mbedtls/library;$(BUILD)/tee/QCBOR/"
else
lib-FLAGS += \
	-DTEE_PLATFORM=$(TEE) \
	-DCMAKE_C_FLAGS='$(DFLAGS) -I$(TAREF_DIR)/build/include -I$(BUILD)/tee/QCBOR/inc \
	  -I$(TAREF_DIR)/api/include'
endif

ifeq ($(TEE),keystone)
lib-FLAGS += \
	-DKEYSTONE_SDK_DIR=${KEYSTONE_SDK_DIR}
endif

.PHONY: lib
lib:
	mkdir -p build/$(TEE)/lib
	cd build/$(TEE)/lib && cmake $(TOPDIR)/lib $(lib-FLAGS) $(cmake-tee-TOOLCHAIN)
	$(MAKE) -C build/$(TEE)/lib DFLAGS=$(DFLAGS)

.PHONY: agent
agent:
	$(MAKE) -C teep-agent-ta DFLAGS=$(DFLAGS) -f $(TEE).mk out-dir=$(BUILD)/agent

.PHONY: broker
broker:
	$(MAKE) -C teep-broker-app TAM_URL=$(TAM_URL) DFLAGS=$(DFLAGS)

.PHONY: hello-tc
hello-tc:
	$(MAKE) -C hello-tc/build-$(PLAT) \
		SOURCE=$(TOPDIR)/hello-tc \
		TAM_URL=$(TAM_URL) DFLAGS=$(DFLAGS)

.PHONY: clean-hello-tc
clean-hello-tc:
	$(MAKE) -C hello-tc/build-keystone SOURCE=$(TOPDIR)/hello-tc clean
	$(MAKE) -C hello-tc/build-optee SOURCE=$(TOPDIR)/hello-tc clean
	$(MAKE) -C hello-tc/build-sgx SOURCE=$(TOPDIR)/hello-tc clean
	$(MAKE) -C hello-tc/build-pc SOURCE=$(TOPDIR)/hello-tc clean

.PHONY: rootfs
rootfs:
	$(MAKE) -C sample rootfs TAM_URL=$(TAM_URL)

.PHONY: install
install:
	$(MAKE) -C sample install prefix=$(prefix) prefix_bin=$(prefix_bin) \
		prefix_lib=$(prefix_lib)

.PHONY: test
test:
	cd $(BUILD)/lib && ctest -V

.PHONY: docs
docs:
	@echo "Updating the directories tree structure"
	bash docs/update_tree_structure.sh
	@echo "Generating doxygen files"
	@doxygen docs/doxygen/Doxyfile
	make -C docs/doxygen/latex
	cp docs/doxygen/latex/refman.pdf docs/teep-device.pdf	
	rm -fr docs/teep-device_readme_html
	mv docs/doxygen/html docs/teep-device_readme_html
	cd docs; tar czf teep-device_readme_html.tar.gz open-readme.html teep-device_readme_html

gen_readme:
	cat docs/overview_of_teep-device.md docs/tree_view_dir.md \
		docs/building_with_docker.md \
		docs/building_notee.md > README.md
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
	$(MAKE) -C sample run-session TAM_URL=$(TAM_URL)

.PHONY: run-qemu
run-qemu: check-tee
	$(MAKE) -C sample run-qemu TAM_URL=$(TAM_URL)

.PHONY: check-tee
ifeq ($(TEE),)
check-tee:
	@echo '$$TEE must be "optee", "keystone", "sgx" or "pc"'
	@false
else
check-tee:
endif

