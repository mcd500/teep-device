

libteep: libteep-host libteep-ree libteep-tee
hello: hello-ta hello-app
teep: teep-agent-ta teep-broker-app
export CFLAGS=-Wno-enum-conversion

libteep-host: libteep-mbedtls-host libteep-libwebsockets-host
libteep-ree: libteep-mbedtls-ree libteep-libwebsockets-ree libteep-libteep-ree
libteep-tee: libteep-mbedtls-tee libteep-libwebsockets-tee

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

libteep-libteep-ree: libteep-mbedtls-ree libteep-libwebsockets-ree
	if [ -z "$($@-DISABLE)" ]; then \
		mkdir -p $(BUILD)/libteep/ree/lib && \
		$(CROSS_COMPILE)gcc $(REE_CFLAGS) -Ilib -c $(LIBTEEP_SRC) -o $(BUILD)/libteep/ree/lib/main.o && \
		$(CROSS_COMPILE)gcc $(REE_LDFLAGS) \
			-shared -Wl,-soname,libteep.so.1 \
			-L$(OPTEE_DIR)/out-br/target/usr/lib \
			$(BUILD)/libteep/ree/lib/main.o -lwebsockets -lmbedx509 -lmbedtls -lmbedcrypto \
			-o $(BUILD)/libteep/ree/lib/libteep.so.1 && \
		ln -sf libteep.so.1 $(BUILD)/libteep/ree/lib/libteep.so; \
	fi

hello-ta: libteep
	$(MAKE) -C $(SOURCE)/hello-ta -f $(PLAT).mk out-dir=$(BUILD)/hello-ta

hello-app: libteep
	$(MAKE) -C $(SOURCE)/hello-app -f $(PLAT).mk out-dir=$(BUILD)/hello-app APP_CFLAGS="$(APP_CFLAGS)" APP_LDFLAGS="$(APP_LDFLAGS)"

teep-agent-ta: libteep
	$(MAKE) -C $(SOURCE)/teep-agent-ta -f $(PLAT).mk out-dir=$(BUILD)/teep-agent-ta

teep-broker-app: libteep
	$(MAKE) -C $(SOURCE)/teep-broker-app -f $(PLAT).mk out-dir=$(BUILD)/teep-broker-app APP_CFLAGS="$(APP_CFLAGS)" APP_LDFLAGS="$(APP_LDFLAGS)"
