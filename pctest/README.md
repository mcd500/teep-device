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
./pctest --tamurl http://{ip_adress_of_tam}:{port}{path} [--jose]  
         [--talist uuid_of_deleting_ta]
```

### Examples

* Install TA without encrypted json message
```bash
./pctest --tamurl http://192.168.11.3:3000/api/tam
```

* Deleting TA without encrypted json message
```bash
./pctest --tamurl http://192.168.11.3:3000/api/tam --talist 8d82573a-926d-4754-9353-32dc29997f74
```

* Install TA with encrypted json message
```bash
./pctest --tamurl http://192.168.11.3:3000/api/tam_jose --jose
```

* Deleting TA with encrypted jsone message
```bash
./pctest --tamurl http://192.168.11.3:3000/api/tam_jose --jose --talist 8d82573a-926d-4754-9353-32dc29997f74
```

Show help
```bash
./pctest --help
```
