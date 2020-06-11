TARGET=hello-app

.PHONY: all
all: $(TARGET)

$(TARGET): main.c
	$(CROSS_COMPILE)gcc -I$(TA_DEV_KIT_DIR)/host_include $(INCLUDES) -L../../optee_client/out/libteec $(LDTEEC) $^ -lteec -o $@

.PHONY: clean
clean:
	rm -f $(TARGET)

.PHONY: update
update:
	scp $(TARGET) root@${HIKEY_IP}:/usr/bin

