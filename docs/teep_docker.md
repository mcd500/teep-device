# TEEP-Device with docker


## Preparation for Docker

For building teep-device with docker, it is required to install docker on Ubuntu.

For the first time users of docker, please have a look on https://docs.docker.com/engine/

The following installation steps is for Ubuntu 20.04

### Installing Docker

```sh
$ sudo apt update
	 
# Next, install a few prerequisite packages which let apt use packages over HTTPS:
$ sudo apt install apt-transport-https ca-certificates curl software-properties-common
	 
# Then add the GPG key for the official Docker repository to your system:
$ curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
	
# Add the Docker repository to APT sources:
$ sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu focal stable"
	 
# This will also update our package database with the Docker packages from the newly added repo.
# Make sure you are about to install from the Docker repo instead of the default Ubuntu repo:
$ apt-cache policy docker-ce
		 
#Finally, install Docker
$ sudo apt install docker-ce
```

### Executing Docker without sudo

By default, the docker command can only be run the root user or by a user in the docker group, which is automatically created during Docker’s installation process. If you attempt to run the docker command without prefixing it with sudo or without being in the docker group, you’ll get an output like this:

```console
docker: Cannot connect to the Docker daemon. Is the docker daemon running on this host?.
```

To avoid typing sudo whenever we run the docker command, add your username to the docker group.

```sh
$ sudo groupadd docker

$ sudo gpasswd -a $USER docker

# Logout and then log-in again to apply the changes to the group
 ```

After you logout and login, you can probably run the docker command without `sudo`

```sh
$ docker run hello-world
```

### Create a docker network tamproto

A docker network named tamproto is required when we run teep device with ta-ref for all targets.
The local network is required to connect with tamproto service running locally.

```sh
$ docker network create tamproto_default 
```


## Pre-built Docker Image details

The following are the docker images that has pre-built and tested binaries of teep-device with ta-ref.
Since this images are already prepared and built already, you can start using it directly without
building the teep-device again.
Make sure you have account on docker-hub. If not please create one on `dockerhub.com`

| Target | docker image |
| ------ | ------ |
| Keystone | trasioteam/teep-dev:keystone |
| OP-TEE | trasioteam/teep-dev:optee |
| Intel SGX | trasioteam/teep-dev:sgx |
| Tamproto | trasioteam/teep-dev:tamproto |
| Doxygen | trasioteam/teep-dev:doxygen |


## Prepartion for building teep-device on docker

### Docker images details for building

If we need to build the teep-device,
docker images with all necessary packages for building teep-device for all three targets are already available.
The details are mentioned below.


| Target | docker image |
| ------ | ------ |
| Keystone | trasioteam/taref-dev:keystone |
| OP-TEE | trasioteam/taref-dev:optee |
| Intel SGX | trasioteam/taref-dev:sgx |
| Doxygen | trasioteam/taref-dev:doxygen |


