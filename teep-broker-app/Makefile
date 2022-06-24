TOPDIR = $(CURDIR)/..
include $(TOPDIR)/conf.mk

SCRIPTS := env.sh get-ip.sh itc.sh rtc.sh showtamurl.sh

out-dir = $(BUILD)/broker

.PHONY: all
all: $(out-dir)/teep-broker-app $(SCRIPTS)

$(SCRIPTS): %: scripts/%.in
	mkdir -p $(BUILD)/scripts
	for i in $< ; do \
		sed -e 's!@TAM_URL@!$(TAM_URL)!' \
			-e 's!@PLAT@!$(PLAT)!' \
			$$i > $(BUILD)/scripts/$@; \
		chmod +x $(BUILD)/scripts/$@; \
	done

CFLAGS = \
	$(REE_CFLAGS) \
	-Wall \
	-I$(BUILD)/tee/QCBOR/inc \
	-I$(TOPDIR)/include \
	-I$(TOPDIR)/lib/include \
	-I$(BUILD)/ree/libwebsockets/include \
	-I$(TOPDIR)/submodule/libwebsockets/include \
	-I$(TOPDIR)/submodule/mbedtls/include \
	-I../teep-agent-ta

OBJ = \
	$(out-dir)/teep-broker.o \
	$(out-dir)/http-lws.o

LDFLAGS = \
	-L$(BUILD)/ree/mbedtls/library \
	-L$(BUILD)/ree/libwebsockets/lib \
	-L$(BUILD)/ree/lib

LIBS = -lwebsockets -lmbedtls -lmbedcrypto -lmbedx509 -lpthread

ifeq ($(PLAT), pc)
OBJ += \
	$(out-dir)/teec-pc.o \
	$(BUILD)/agent/agent.a

LDFLAGS += \
	-L$(BUILD)/lib \
	-L$(BUILD)/tee/QCBOR

LIBS +=  -lteep -lqcbor -lteesuit -lteecbor

ifeq ($(shell uname),Linux)
LIBS += -lcap
endif

endif

ifeq ($(PLAT), keystone)
OBJ += \
	$(out-dir)/teec-keystone.o
endif

ifeq ($(PLAT), sgx)
OBJ += \
	$(out-dir)/teec-sgx.o

LIBS += -lcap
endif

$(out-dir)/teep-broker-app: $(OBJ)
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)g++  $(CFLAGS) $(LDFLAGS) $(REE_LDFLAGS) $^ -o $@ $(LIBS) $(REE_LIBS)

$(out-dir)/%.o: %.c
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)gcc $(CFLAGS) $< -c -o $@

$(out-dir)/%.o: %.cpp
	mkdir -p $(out-dir)
	$(CROSS_COMPILE)g++ $(CFLAGS) $< -c -o $@

.PHONY: clean
clean:
	rm -f $(out-dir)/teep-broker-app
	rm -f $(out-dir)/*.o

$(out-dir)/teep-broker.o: teep-broker.c http.h
$(out-dir)/http-lws.o: http-lws.c http.h

$(out-dir)/teec-pc.o: teec-pc.c
$(out-dir)/teec-keystone.o: teec-keystone.cpp
$(out-dir)/teec-sgx.o: teec-sgx.cpp
