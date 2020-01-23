.PHONY: libaistotrp_seq
libaistotrp_seq:
	make -C libaistotrp TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE)
	make -C aist-otrp-testapp TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE)
	make -C aist-otrp-test-ta-client TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE)

.PHONY: clean
clean:
	make -C libaistotrp TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean
	make -C aist-otrp-testapp TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean
	make -C aist-otrp-test-ta-client TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean
	make -C ta-aist-otrp TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean
	make -C ta-aist-test TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR) CROSS_COMPILE=$(CROSS_COMPILE) clean
