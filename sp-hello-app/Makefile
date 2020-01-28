TARGET=sp-hello-app

.PHONY: all
all: $(TARGET)

$(TARGET): main.c
	$(CROSS_COMPILE)gcc -I$(TA_DEV_KIT_DIR)/host_include -I../../optee_client/public -L../../optee_client/out/libteec $^ -lteec -o $@

.PHONY: clean
clean:
	rm -f sp-hello-app

.PHONY: update
update:
	scp sp-hello-app root@${HIKEY_IP}:/usr/bin

