.PHONY: aist-otrp-testapp
aist-otrp-testapp:
	#
	# aist-otrp-testapp (REE test application includes libaistotrp)
	#
	$(CROSS_COMPILE)gcc $(CFLAGS) -I../libaistotrp/libwebsockets/build/include \
		-I$(TA_DEV_KIT_DIR) -I../libaistotrp/mbedtls/include\
		-I../../optee_client/out/export/include \
		-I../libaistotrp/lib -c main.c -o main.o
	$(CROSS_COMPILE)gcc -Llibwebsockets/build/lib -Lmbedtls-build/lib \
		-L../../optee_client/out/libteec -Llib -L../libaistotrp/lib \
		-L ../libaistotrp/mbedtls-build/library -L../libaistotrp/libwebsockets/build/lib \
		main.o \
		-lteec -laistotrp -lwebsockets -lmbedx509 -lmbedtls -l mbedcrypto \
		-o aist-otrp-testapp

