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

# TARGET : all
.PHONY: all
all: check-tee submodule lib agent broker hello-tc rootfs

# TARGET : clean
# Clean all the built binaries and entire build folder
.PHONY: clean
clean: clean-hello-tc clean-docs
	rm -rf build

# TARGET : clean_nolibs
# Clean all the built binaries except submodules (mbedtls & libwebsockets)
# Entire build folder is not deleted
.PHONY: clean_nolibs
clean_nolibs: clean-hello-tc clean-docs
	# Delete all the folders except "ree" which has the submodule built binaries
	cd $(BUILD) && ls | grep -xv "ree" | xargs rm -rf


# TARGET : submodule
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

# TARGET : lib      
# Build the lib only when source code is updated
LIB_SRC := \
                $(shell find lib -name "Makefile") \
                $(shell find lib -name "*.c") \
                $(shell find lib -name "*.cpp") \
                $(shell find lib -name "*.h") 
#                 $(shell find lib -name "*.txt") \
#                 $(shell find lib -name "*.suit")

LIB_OBJ = $(BUILD)/lib/log \
			 $(BUILD)/lib/cbor \
			 $(BUILD)/lib/cose \
			 $(BUILD)/lib/suit \
			 $(BUILD)/lib/teep

.PHONY: lib
lib $(LIB_OBJ): $(LIB_SRC)
	mkdir -p build/$(TEE)/lib
	cd build/$(TEE)/lib && cmake $(TOPDIR)/lib $(lib-FLAGS) $(cmake-tee-TOOLCHAIN)
	$(MAKE) -C build/$(TEE)/lib DFLAGS=$(DFLAGS)

# TARGET : broker
# Build the teep-broker-app only when source code is updated
BROKER_SRC := \
                $(shell find teep-broker-app -name "Makefile") \
                $(shell find teep-broker-app -name "*.in") \
                $(shell find teep-broker-app -name "*.cpp") \
                $(shell find teep-broker-app -name "*.c") \
                $(shell find teep-broker-app -name "*.h")

BROKER_OBJ = $(BUILD)/broker/http-lws.o \
			 $(BUILD)/broker/teec-keystone.o  \
			 $(BUILD)/broker/teep-broker.o  \
			 $(BUILD)/broker/teep-broker-app

# TARGET : agent
# Build the teep-agent-ta only when source code is updated
AGENT_SRC := \
                $(shell find teep-agent-ta -name "*.mk") \
                $(shell find teep-agent-ta -name "*.c") \
                $(shell find teep-agent-ta -name "*.h")

AGENT_OBJ = $(BUILD)/teep-agent-ta/ta-store.o \
			 $(BUILD)/teep-agent-ta/tools.o  \
			 $(BUILD)/teep-agent-ta/teep-agent-ta.o  \
			 $(BUILD)/teep-agent-ta/teep-agent-ta

.PHONY: agent
agent $(AGENT_OBJ): $(AGENT_SRC)
	$(MAKE) -C teep-agent-ta DFLAGS=$(DFLAGS) -f $(TEE).mk out-dir=$(BUILD)/agent


# TARGET : broker
# Build the teep-broker-app only when source code is updated
BROKER_SRC := \
                $(shell find teep-broker-app -name "Makefile") \
                $(shell find teep-broker-app -name "*.in") \
                $(shell find teep-broker-app -name "*.cpp") \
                $(shell find teep-broker-app -name "*.c") \
                $(shell find teep-broker-app -name "*.h")

BROKER_OBJ = $(BUILD)/broker/http-lws.o \
			 $(BUILD)/broker/teec-keystone.o  \
			 $(BUILD)/broker/teep-broker.o  \
			 $(BUILD)/broker/teep-broker-app

.PHONY: broker
broker $(BROKER_OBJ): $(BROKER_SRC)
		$(MAKE) -C teep-broker-app TAM_URL=$(TAM_URL) DFLAGS=$(DFLAGS)

# TARGET : hello-tc
# Build the hello-tc only when source code is updated
HELLO_TC_SRC := \
                $(shell find hello-tc -name "Makefile") \
                $(shell find hello-tc -name "*.cpp") \
                $(shell find hello-tc -name "*.c") \
                $(shell find hello-tc -name "*.h") \
                $(shell find hello-tc -name "*.mk") \
                $(shell find hello-tc -name "*.in") \
                $(shell find hello-tc -name "*.py") \
                $(shell find hello-tc -name "*.lds") \
                $(shell find hello-tc -name "*.pem") \

HELLO_TC_OBJ = $(BUILD)/hello-tc/App-$(PLAT).o 

.PHONY: hello-tc
hello-tc $(HELLO_TC_OBJ): $(HELLO_TC_SRC)
	$(MAKE) -C hello-tc/build-$(PLAT) \
		SOURCE=$(TOPDIR)/hello-tc \
		TAM_URL=$(TAM_URL) DFLAGS=$(DFLAGS)

.PHONY: clean-hello-tc
clean-hello-tc:
	$(MAKE) -C hello-tc/build-keystone SOURCE=$(TOPDIR)/hello-tc clean
	$(MAKE) -C hello-tc/build-optee SOURCE=$(TOPDIR)/hello-tc clean
	$(MAKE) -C hello-tc/build-sgx SOURCE=$(TOPDIR)/hello-tc clean
	$(MAKE) -C hello-tc/build-pc SOURCE=$(TOPDIR)/hello-tc clean


# TARGET : rootfs
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


# TARGET : docs
# For generation doxygen documentation

docs/tree_view_dir.md:
	@echo "Updating the directories tree structure"
	bash docs/update_tree_structure.sh

.PHONY: docs
docs:
	@echo "Generating doxygen files"
	@doxygen docs/doxygen/Doxyfile
	make -C docs/doxygen/latex
	cp docs/doxygen/latex/refman.pdf docs/teep-device.pdf	
	rm -fr docs/teep-device_readme_html
	mv docs/doxygen/html docs/teep-device_readme_html
	cd docs; tar czf teep-device_readme_html.tar.gz open-readme.html teep-device_readme_html

gen_readme: docs/tree_view_dir.md
	cat docs/overview_of_teep-device.md docs/tree_view_dir.md \
		docs/build-environment.md \
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


# TARGET : run-sample-session
# For running sample session on respective targets
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

