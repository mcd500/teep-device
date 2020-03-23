IP_ADDR := $(shell ../scripts/get-ip.sh)
IP_SEG  := $(shell echo ${IP_ADDR} | cut --complement -d'.' -f 4)
IP_FILE := $(shell echo $(IP_SEG) > tam_ip)

MBEDTLS_BDIR := build-mbedtls
LWS_M_BDIR := build-lws-mbed

SCRIPT := ita dta ista dsta iota dota showtamurl

.PHONY: all
all: teep-broker-app $(SCRIPT)

$(SCRIPT): %: tam_ip %.sh
	sed -e s/127\.0\.\0\.1/$(IP_ADDR)/g < $^ > $@
	sed -e s/IPSEG/$(IP_SEG)/g < iup.sh > iup
	chmod u+x $@ iup

teep-broker-app: main.o
	#
	# teep-broker-app (REE test application includes libteep)
	#
	$(CROSS_COMPILE)gcc -Llibwebsockets/build/lib -Lmbedtls-build/lib \
		-L../../optee_client/out/libteec -Llib -L../libteep/lib \
		-L ../libteep/$(MBEDTLS_BDIR)/library -L../libteep/$(LWS_M_BDIR)/lib \
		$^ \
		-lteec -lteep -lwebsockets -lmbedx509 -lmbedtls -l mbedcrypto \
		-o $@

main.o: main.c
	$(CROSS_COMPILE)gcc $(CFLAGS) -I../libteep/libwebsockets/include \
		-I../libteep/$(LWS_M_BDIR)/include \
		-I$(TA_DEV_KIT_DIR) -I../libteep/mbedtls/include\
		-I../../optee_client/out/export/include \
		-I../libteep/lib -c $< -o $@


.PHONY: clean
clean:
	rm -f *.o teep-broker-app $(SCRIPT) iup
