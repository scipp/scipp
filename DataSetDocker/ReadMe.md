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

`ip = 127.0.0.1`

## For windows:

`ip = docker-machine ip $(docker-machine active)`

# To access the jupyter notebook:

Now the console with jupyter-lab server is running.

1. Copy the address with token from the console into your browser:

``` 
<ip>:8888/?token=1708633ec3f24c945c8cc18ce0ca40f90d2ac94a0a55119f
```

The token is different every time and is printed when starting docker.
