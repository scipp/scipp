# Using Dataset under Docker

## Install Docker

Follow the instructions [here](https://docs.docker.com/install/) to
install Docker CE on your system.

### Linux extra

Add your user to the `docker` group (to avoid having to use `sudo`
with Docker commands, note you'll have to logout and back in for
these changes to take effect):
```sh
sudo usermod -aG docker $(whoami)
```

If the Docker daemon is not running by default you may need to
enable (and manually start) it:
```
sudo systemctl enable docker
sudo systemctl start docker
```

## To build the image

From the directory containing Dockerfile launch:

```sh
docker build --tag dataset .
```

## To start a container

### For Linux:

```sh
docker run -p 8888:8888 dataset
```

### For Windows/MacOS

```sh
docker run -p $(docker-machine ip $(docker-machine active)):8888:8888 dataset
```
## Get the IP of running container

### For Linux

```
ip = 127.0.0.1
```

### For windows

```
ip = docker-machine ip $(docker-machine active)
```

## To access the Jupyter notebook

In your browser type:
``` 
<ip>:8888
```
In the home folder there is a notebook `update_examples` which can be 
used for loading the "fresh" examples from github by executing it: 
open the notebook; press `shift+enter`. 
Examples are located in `work/examples`.
