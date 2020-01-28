.PHONY: all
all: teep-broker-app

teep-broker-app: main.o
	#
	# teep-broker-app (REE test application includes libteep)
	#
	$(CROSS_COMPILE)gcc -Llibwebsockets/build/lib -Lmbedtls-build/lib \
		-L../../optee_client/out/libteec -Llib -L../libteep/lib \
		-L ../libteep/mbedtls-build/library -L../libteep/libwebsockets/build/lib \
		$^ \
		-lteec -lteep -lwebsockets -lmbedx509 -lmbedtls -l mbedcrypto \
		-o $@

main.o: main.c
	$(CROSS_COMPILE)gcc $(CFLAGS) -I../libteep/libwebsockets/include \
		-I../libteep/libwebsockets/build/include \
		-I$(TA_DEV_KIT_DIR) -I../libteep/mbedtls/include\
		-I../../optee_client/out/export/include \
		-I../libteep/lib -c $< -o $@


.PHONY: clean
clean:
	rm -f *.o teep-broker-app
