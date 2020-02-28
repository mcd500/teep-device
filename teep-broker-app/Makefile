ETHER := $(shell ip link | awk -F: '$$0 !~ "lo|vir|wl|pp|docker|br|^[^0-9]"{print $$2;getline}')
IP_ADDR := $(shell ifconfig $(ETHER) | awk '/inet / {gsub("addr:","",$$2); print $$2}')

MBEDTLS_BDIR := build-mbedtls
LWS_M_BDIR := build-lws-mbed

SCRIPT := ita dta ista dsta iota dota showtamurl

.PHONY: all
all: teep-broker-app $(SCRIPT)

$(SCRIPT): %: %.sh
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
