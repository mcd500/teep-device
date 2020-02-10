# TEEP over HTTP broker test on PC

## prepare libs

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

```bash
./pctest --tamurl http://{ip_adress_of_tam}:{port}/{api_path}
```

Show help
```bash
./pctest --help
```
