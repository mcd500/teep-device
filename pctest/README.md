# TEEP over HTTP broker test on PC

## prepare libs

### libs on macos

```bash
brew intall libwebsockets
```

### libs on ubuntu 18.04

```bash
git clone http://192.168.100.100/vc707/libwebsockets
cd libwebsockets
sudo apt-get install libssl-dev libmbedtls-dev
mkdir build
cd build
cmake -DLWS_WITH_SSL=1 -DLWS_WITH_MBEDTLS=1 -DLWS_WITH_JOSE=1 ..
make -j `nproc`
sudo ldconfig `pwd`/lib
cd ../..
```

## build

```bash
make
```

## run

```bash
./pctest --tamurl http://{ip_adress_of_tam}:{port}
```

Show help
```bash
./pctest --help
```
