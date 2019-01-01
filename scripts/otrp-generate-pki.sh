#!/bin/bash
  
C="/C=JP/ST=Kantou/L=Tokyo/O=AIST/OU=AIST OTrP"
RSAKEYLEN=4096
EC_CURVE=P-521
SIG_MD=-sha512

# may be either "rsa" or "ec"
# by default, rootca: EC, ca: EC, signed cert: RSA
# this is to exercise both EC and RSA support
#
CA_CRYPTO=ec
SIGN_CRYPTO=rsa

function usage
{
        echo "Usage:"
	echo
        echo "   $0 rootca [sp|tam|tee] <dir>"
	echo "        eg, $0 rootca sp /root/sp-rootca"
	echo ""
        echo "   $0 ca <path to rootca cert> <dir to create ca in> <ca name>"
	echo "        eg, $0 ca /root/sp-rootca /root/sp-ca 1"
	echo ""
        echo "   $0 sign <dir with ca credentials> <dir to create cert in> <cert name>"
	echo "        eg, $0 sign /root/sp-ca1 /root/sp sp-xbank"

        exit 1
}

if [ -z "$1" -o -z "$2" ] ; then
        usage
fi

case $1 in
# root certificate
rootca)
	RCADIR="$3/$2-rootca"
	mkdir -p "$3"

# RSA (uncomment to enable)
#        openssl genrsa -out $RCADIR-rsa-key.pem $RSAKEYLEN && \
#	openssl req -x509 -new -nodes -key $RCADIR-rsa-key.pem \
#		    $SIG_MD -days 30000 -out $RCADIR-rsa-cert.pem \
#		    -subj "$C/CN=$2-rootca"

# EC
	openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:$EC_CURVE \
			-out $RCADIR-ec-key.pem
	openssl req -x509 -new -key $RCADIR-ec-key.pem -subj "$C/CN=$2-rootca" \
			-out $RCADIR-ec-cert.pem

	echo
        echo "Generated new rootca certs and keys in $RCADIR-[rsa,ec]-[key,cert].pem"
	echo
	echo "The certs are public, but the after creating the CAs, the keys should"
	echo " be copied to a USB stick stored in a secure location and deleted.  They"
	echo " will only be needed again if a replacement CA must be generated."
	echo
        ;;

# intermediate CA cert
ca)
	if [ -z "$3" -o -z "$4" ] ; then
	        usage
	fi

	if [ ! -e "$2" ] ; then
		echo "Can't open $2"
		exit 1
	fi

	RCA_FUNCTION=`openssl x509 -in "$2" -text | grep CN\ = | grep Subject: | head -n 1 | sed "s/.*CN\ =\ //g"`

	case $RCA_FUNCTION in
	sp-rootca)
		F=sp
		;;
	tam-rootca)
		F=tam
		;;
	tee-rootca)
		F=tee
		;;
	*)
		echo "$RCA_FUNCTION: rootca doing the signing must be for sp, tam or tee"
		exit 1
	esac

	RCADIR="$3/$F-ca-$4"
	mkdir -p "$3"

	if [ $CA_CRYPTO == "rsa" ] ; then
	        openssl genrsa -out $RCADIR-rsa-key.pem $RSAKEYLEN
	else
		openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:$EC_CURVE \
                        -out $RCADIR-ec-key.pem
	fi
	if [ $? -ne 0 ]; then
		exit 1
	fi

        openssl req -new -nodes -key $RCADIR-$CA_CRYPTO-key.pem -out $RCADIR.csr -subj "$C/CN=$F-ca" && \
        openssl x509 -req -in $RCADIR.csr \
                -CA $2 \
                -CAkey `echo -n $2 | sed "s/-cert.pem/-key.pem/g"` \
                -CAcreateserial \
                -out $RCADIR-$CA_CRYPTO-cert.pem \
                -days 20000 $SIG_MD && \
        rm -f $RCADIR.csr

        echo
        echo "Generated $RCADIR-key.pem:  CA private key"
        echo "          $RCADIR-cert.pem: CA public cert"
        ;;

#
# the signed certificate that's actually used
# we add the intermediate CA cert to the signed cert
#
sign)
	if [ -z "$3" -o -z "$4" ] ; then
	        usage
	fi

	if [ ! -e "$2" ] ; then
		echo "Can't open $2"
		exit 1
	fi

	RCA_FUNCTION=`openssl x509 -in "$2" -text | grep CN\ = | grep Subject: | head -n 1 | sed "s/.*CN\ =\ //g"`

	case $RCA_FUNCTION in
	sp-ca)
		F=sp
		;;
	tam-ca)
		F=tam
		;;
	tee-ca)
		F=tee
		;;
	*)
		echo "$RCA_FUNCTION: signing ca must be for sp, tam or tee - not the rootca for those either"
		exit 1
	esac

	RCADIR="$3/$F-$4"
	mkdir -p "$3"

	if [ $SIGN_CRYPTO == "rsa" ] ; then
	        openssl genrsa -out $RCADIR-rsa-key.pem $RSAKEYLEN
	else
		openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:$EC_CURVE \
                        -out $RCADIR-ec-key.pem
	fi
	if [ $? -ne 0 ]; then
		exit 1
	fi


        openssl req -new -nodes -key $RCADIR-$SIGN_CRYPTO-key.pem -out $RCADIR.csr -subj "$C/CN=$F-$4" && \
        openssl x509 -req -in $RCADIR.csr \
                -CA $2 \
                -CAkey `echo -n $2 | sed "s/-cert.pem/-key.pem/g"` \
                -CAcreateserial \
                -out $RCADIR-$SIGN_CRYPTO-cert.pem \
                -days 10000 $SIG_MD && \
        rm -f $RCADIR.csr

	# add the intermediate cert in
	cat "$2" $RCADIR-$SIGN_CRYPTO-cert.pem > $RCADIR-$SIGN_CRYPTO-cert-plus-intermediate-cert.pem

        echo
        echo "Generated $RCADIR-key.pem:  $F private key"
        echo "          $RCADIR-cert.pem: $F public cert"

        ;;

*)
        usage
esac

