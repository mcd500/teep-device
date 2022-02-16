ifndef TUTORIAL_TA_UUID
$(error TUTORIAL_TA_UUID is not set)
endif

ifndef TAREF_DIR
$(error TAREF_DIR is not set)
endif

BINARY = $(TUTORIAL_TA_UUID)
CPPFLAGS = \
	-I$(TAREF_DIR)/build/include \
	-I$(TAREF_DIR)/build/include/api \
	-I$(TAREF_DIR)/build/include/gp \
	-Dtee_printf=printf

-include $(TA_DEV_DIR)/mk/ta_dev_kit.mk

.PHONY: clean
clean:
	rm -rf *.o .*.o.* *.dmp *.elf *.map $(BINARY).ta ta.lds .ta.ld.d

