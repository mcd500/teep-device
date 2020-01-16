all:
	$(CROSS_COMPILE)gcc -I$(TA_DEV_KIT_DIR)/host_include -I../../optee_client/public -L../../optee_client/out/libteec main.c -lteec -o aist-otrp-test-ta-client

clean:
	rm -f aist-otrp-test-ta-client

update:
	scp aist-otrp-test-ta-client root@${HIKEY_IP}:/usr/bin

