# To build the image:

From the directory containing Dockerfile launch:

```sh
docker build --tag dataset .
```

# To start docker: 

```sh
docker run -p 8888:8888 dataset
```
# To access the jupyter:

Now you have the console with jupyter-lab server running.

1. Copy the address with token from the console and put this line in your browser:

``` 
127.0.0.1:8888/?token=1708633ec3f24c945c8cc18ce0ca40f90d2ac94a0a55119f
```
