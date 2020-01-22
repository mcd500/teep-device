# TEEP over HTTP broker test on PC

## prepare libs

### libs on macos

```bash
brew intall libwebsockets
```

### libs on ubuntu 18.04

```bash
git clone https://github.com/warmcat/libwebsockets.git
cd libwebsockets
git checkout v3.1-stable
sudo apt-get install libssl-dev
mkdir build
cd build
cmake ..
make
sudo make install
sudo ldconfig
cd ../..
```

## build

```bash
make
```

## run

```bash
./pctest
```

```bash
./pctest "{\"GetDeviceStateResponse\": {}}"
```

```bash
./pctest "{\"InstallTAResponse\": {}}"
```
