

libteep: libteep-host libteep-ree libteep-tee
hello: hello-ta hello-app
teep: teep-agent-ta teep-broker-app

libteep-host: libteep-mbedtls-host libteep-libwebsockets-host libteep-QCBOR-host
libteep-ree: libteep-mbedtls-ree libteep-libwebsockets-ree libteep-QCBOR-ree
libteep-tee: libteep-mbedtls-tee libteep-libwebsockets-tee libteep-QCBOR-tee libteep-libteep-tee libteesuit-tee

libteep-mbedtls-host:
	if [ -z "$($@-DISABLE)" ]; then \
		mkdir -p $(BUILD)/libteep/host/mbedtls && \
		cd $(BUILD)/libteep/host/mbedtls && \
		cmake $($@-FLAGS) $(SOURCE)/libteep/mbedtls && \
		make -j `nproc`; \
	fi

libteep-mbedtls-ree:
	if [ -z "$($@-DISABLE)" ]; then \
		mkdir -p $(BUILD)/libteep/ree/mbedtls && \
		cd $(BUILD)/libteep/ree/mbedtls && \
		cmake $($@-FLAGS) $(SOURCE)/libteep/mbedtls && \
		make -j `nproc`; \
	fi

libteep-mbedtls-tee:
	if [ -z "$($@-DISABLE)" ]; then \
		mkdir -p $(BUILD)/libteep/tee/mbedtls && \
		cd $(BUILD)/libteep/tee/mbedtls && \
		cmake $($@-FLAGS) $(SOURCE)/libteep/mbedtls && \
		make -j `nproc`; \
	fi

libteep-libwebsockets-host:
	if [ -z "$($@-DISABLE)" ]; then \
		mkdir -p $(BUILD)/libteep/host/libwebsockets && \
		cd $(BUILD)/libteep/host/libwebsockets && \
		cmake $($@-FLAGS) $(SOURCE)/libteep/libwebsockets && \
		make -j `nproc`; \
	fi

libteep-libwebsockets-ree:
	if [ -z "$($@-DISABLE)" ]; then \
		mkdir -p $(BUILD)/libteep/ree/libwebsockets && \
		cd $(BUILD)/libteep/ree/libwebsockets && \
		cmake $($@-FLAGS) $(SOURCE)/libteep/libwebsockets && \
		make -j `nproc`; \
	fi

libteep-libwebsockets-tee:
	if [ -z "$($@-DISABLE)" ]; then \
		mkdir -p $(BUILD)/libteep/tee/libwebsockets && \
		cd $(BUILD)/libteep/tee/libwebsockets && \
		cmake $($@-FLAGS) $(SOURCE)/libteep/libwebsockets && \
		make -j `nproc`; \
	fi

libteep-QCBOR-host:
	if [ -z "$($@-DISABLE)" ]; then \
		mkdir -p $(BUILD)/libteep/host && \
		rm -rf $(BUILD)/libteep/host/QCBOR && \
		cp -r $(SOURCE)/libteep/QCBOR $(BUILD)/libteep/host && \
		make -C $(BUILD)/libteep/host/QCBOR $($@-FLAGS); \
	fi

libteep-QCBOR-ree:
	if [ -z "$($@-DISABLE)" ]; then \
		mkdir -p $(BUILD)/libteep/ree && \
		rm -rf $(BUILD)/libteep/ree/QCBOR && \
		cp -r $(SOURCE)/libteep/QCBOR $(BUILD)/libteep/ree; \
		make -C $(BUILD)/libteep/ree/QCBOR $($@-FLAGS); \
	fi

libteep-QCBOR-tee:
	if [ -z "$($@-DISABLE)" ]; then \
		mkdir -p $(BUILD)/libteep/tee && \
		rm -rf $(BUILD)/libteep/tee/QCBOR && \
		cp -r $(SOURCE)/libteep/QCBOR $(BUILD)/libteep/tee; \
		make -C $(BUILD)/libteep/tee/QCBOR $($@-FLAGS); \
	fi

libteep-libteep-tee: libteep-mbedtls-tee libteep-libwebsockets-tee libteep-QCBOR-tee
	if [ -z "$($@-DISABLE)" ]; then \
		mkdir -p $(BUILD)/libteep/tee/lib && \
		make -C $(SOURCE)/libteep/lib build=$(BUILD)/libteep/tee/lib CFLAGS=$(TEE_CFLAGS); \
	fi

libteesuit-tee:
	if [ -z "$($@-DISABLE)" ]; then \
		mkdir -p $(BUILD)/libteep/tee/libteesuit && \
		cd $(BUILD)/libteep/tee/libteesuit && \
		cmake $($@-FLAGS) $(SOURCE)/suit && \
		make; \
	fi


hello-ta: libteep
	$(MAKE) -C $(SOURCE)/hello-ta -f $(PLAT).mk out-dir=$(BUILD)/hello-ta

hello-app: libteep
	$(MAKE) -C $(SOURCE)/hello-app -f $(PLAT).mk out-dir=$(BUILD)/hello-app APP_CFLAGS="$(APP_CFLAGS)" APP_LDFLAGS="$(APP_LDFLAGS)"

teep-agent-ta: libteep
	$(MAKE) -C $(SOURCE)/teep-agent-ta -f $(PLAT).mk BUILD=$(BUILD) out-dir=$(BUILD)/teep-agent-ta

teep-broker-app: libteep
	$(MAKE) -C $(SOURCE)/teep-broker-app -f $(PLAT).mk out-dir=$(BUILD)/teep-broker-app APP_CFLAGS="$(APP_CFLAGS)" APP_LDFLAGS="$(APP_LDFLAGS)"
