# TEEP over HTTP broker test on PC

## prepare libs

### libs on macos

```bash
brew intall libwebsockets
```

### libs on ubuntu 18.04

```bash
sudo apt-get purge libmbedtls* libmbedcrypto* libmbedx509*
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
