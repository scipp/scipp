# Installing and configuring docker

## Ubuntu 18.04

```
sudo apt install docker.io
sudo usermod -aG docker $(whoami)
# Now logout and login again for the permission changes to take effect
sudo systemctl start docker
sudo systemctl enable docker
```

# To build the image:

From the directory containing Dockerfile launch:

```sh
docker build --tag dataset .
```

# To start docker: 

## For Linux:

```sh
docker run -p 8888:8888 dataset
```

## For Windows/MacOS:

```sh
docker run -p $(docker-machine ip $(docker-machine active)):8888:8888 dataset
```
# Get the ip of running docker:

## For Linux:

```
ip = 127.0.0.1
```

## For windows:

```
ip = docker-machine ip $(docker-machine active)
```

# To access the jupyter notebook in your browser type:

``` 
<ip>:8888
```

