all: main.c
	$(CROSS_COMPILE)gcc -I$(TA_DEV_KIT_DIR)/host_include -I../../optee_client/public -L../../optee_client/out/libteec main.c -lteec -o sp-hello-app

clean:
	rm -f sp-hello-app

update:
	scp sp-hello-app root@${HIKEY_IP}:/usr/bin

