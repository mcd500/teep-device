CPPFLAGS += -DTEE_TA
CFLAGS += $(TA_CFLAGS)
LDADD += $(TA_LDFLAGS) -lteep -lqcbor
BINARY = $(TEE_AGENT_UUID)

libnames += websockets

-include $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk

$(out-dir)/$(BINARY).elf: $(BUILD)/libteep/tee/QCBOR/libqcbor.a \
		*.c *.h

.PHONY: clean
clean:
	rm -rf *.o .*.o.* *.dmp *.elf *.map $(BINARY).ta ta.lds .ta.ld.d

.PHONY: clean_ta_file
clean_ta_file:
	rm -f $(BINARY).ta
