# TEEP over HTTP broker test on PC

## preparation

Uninstall libraries from local machine to prevent conflicts
which we are going to use from the latest sources.

```bash
sudo apt-get purge libwebsockets* libmbedtls* libmbedcrypto* libmbedx509*
```

## build

```bash
make
```

## run

Usage:
```sh
./teep-broker-app --tamurl http://{ip_adress_of_tam}:{port}{path} [--jose]
         [--talist uuid_of_deleting_ta]
```

### Examples

* Install TA without encrypted json message
```bash
./teep-broker-app --tamurl http://192.168.11.3:3000/api/tam
```

* Deleting TA without encrypted json message
```bash
./teep-broker-app --tamurl http://192.168.11.3:3000/api/tam --talist 8d82573a-926d-4754-9353-32dc29997f74
```

* Install TA with encrypted json message
```bash
./teep-broker-app --tamurl http://192.168.11.3:3000/api/tam_jose --jose
```

* Deleting TA with encrypted jsone message
```bash
./teep-broker-app --tamurl http://192.168.11.3:3000/api/tam_jose --jose --talist 8d82573a-926d-4754-9353-32dc29997f74
```

* Install TA with encrypted json through OTrP
```bash
./teep-broker-app --tamurl http://192.168.11.3:3000/api/tam_jose -p otrp --jose
```

* Deleting TA with encrypted json through OTrP
```bash
./teep-broker-app --tamurl http://192.168.11.3:3000/api/tam_jose_delete -p otrp --jose --talist 8d82573a-926d-4754-9353-32dc29997f74
```

Show help
```bash
./teep-broker-app --help
```

## Scripts on PC

There are some helper scripts to make it easier to type ane run.

* Run Install TA without encrypted json message
```sh
./ita
```

* Run Deleting TA without encrypted json message
```sh
./dta
```

* Run Install TA with encrypted json message
```sh
./ista
```

* Run Deleting TA with encrypted jsone message
```sh
./dsta
```

* Install TA with encrypted json through OTrP
```sh
./iota
```


* Deleting TA with encrypted json through OTrP
```sh
./dota
```
