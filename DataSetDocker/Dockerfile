ARG BASE_CONTAINER=jupyter/base-notebook
FROM $BASE_CONTAINER

LABEL maintainer="Igor Gudich <igor.gudich@esss.se>"

USER root

#installing mantid-framework
RUN apt-get update
RUN apt-get install -y libglu1-mesa
RUN conda install -c conda-forge libglu
RUN conda install -c mantid poco 
RUN conda install -c mantid nexus 
RUN conda install -c mantid/label/nightly mantid-framework

#installing dataset dependencies
RUN conda install --quiet --yes \
    'xarray' && \ 
    fix-permissions $CONDA_DIR && \
    fix-permissions /home/$NB_USER

#installing dataset 
# cmake and clang to compile source code
RUN apt-get install -y software-properties-common
RUN add-apt-repository ppa:mhier/libboost-latest && \
    apt-get update && \
    apt-get install -y cmake && \
    apt-get install -y g++  && \
    apt-get install -y mercurial && \
    apt-get install -y libboost1.67-dev && \
    apt-get install -y git && \
    rm -rf /var/lib/apt/lists/*

RUN cd /tmp && \
    git clone https://github.com/mantidproject/dataset.git && \
    cd ./dataset && \
    git submodule init && \
    git submodule update && \
    mkdir /tmp/dataset_build && \
    cd /tmp/dataset_build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/conda/lib/python3.6/site-packages /tmp/dataset && \
    make -j $(lscpu -p | egrep -v '^#' | sort -u -t, -k 2,4 | wc -l)

USER root

RUN cd /tmp/dataset_build && \
    make install 

RUN sudo apt-get purge --auto-remove -y libboost1.67-dev 

#installing plotly and jupyter extension to visualize dataset
RUN conda install --quiet --yes -c plotly plotly=3.6.1

RUN conda install --quiet --yes jupyterlab=0.35 "ipywidgets>=7.2"
RUN jupyter labextension install @jupyterlab/plotly-extension@0.18.1

#downgrade tornado for correct work
RUN conda install --quiet --yes "tornado<5.0.0"

#installing tools to convert python scripts to notebooks
RUN conda install --quiet --yes pip
RUN pip install py2nb

RUN chown -R $NB_USER /tmp/dataset

# Creating scripts for updating and making examples
ARG UPD_EX=/usr/local/bin/update_repo_and_make_examples.sh
ARG EX_DIR=/home/$NB_USER/work/examples

RUN touch $UPD_EX && \
    chmod a+x $UPD_EX &&\
    echo "#!/bin/bash" >> $UPD_EX && \
    echo "cd /tmp/dataset && git reset --hard && git clean -fd && git checkout . && git pull" >> $UPD_EX && \
    echo "find /tmp/dataset/python/examples -name '*.py' -exec py2nb {} \; ;" >> $UPD_EX && \
    echo "if [ ! -d $EX_DIR ]; then mkdir /home/$NB_USER/work/examples; fi;" >> $UPD_EX && \
    echo "if [ -d /tmp/dataset/python/examples ] && [ -d /home/$NB_USER/work/examples ]; then find /tmp/dataset/python/examples -name '*.ipynb' -exec mv {} /home/$NB_USER/work/examples \; ; fi;" >> $UPD_EX && \
    echo "chmod -R a+rwx /home/$NB_USER/work" >> $UPD_EX && \
    echo "chown -R $NB_USER /home/$NB_USER/work" >> $UPD_EX && \
    echo "exit 0" >> $UPD_EX

#avoid using passwords for jupyter lab
RUN sed -i "s/jupyter notebook/jupyter notebook --NotebookApp.token='' --NotebookApp.password=''/" /usr/local/bin/start-notebook.sh

ARG PG3_4844_HASH=d5ae38871d0a09a28ae01f85d969de1e
ARG PG3_4866_HASH=3d543bc6a646e622b3f4542bc3435e7e

# Copy datafiles NOTE: files should be in the docker build directory
RUN mkdir $EX_DIR && \
    curl http://198.74.56.37/ftp/external-data/MD5/$PG3_4844_HASH --output $EX_DIR/PG3_4844_event.nxs && \
    curl http://198.74.56.37/ftp/external-data/MD5/$PG3_4866_HASH --output $EX_DIR/PG3_4866_event.nxs && \
    chown -R $NB_USER /home/$NB_USER/work

USER $NB_UID

# update and make examples
RUN bash -x $UPD_EX

# creating special notebook for updating examples
ARG UPD_NB=/home/$NB_USER/update_examples

RUN touch $UPD_NB.py && \
    echo "#|" >> $UPD_NB.py && \
    echo "%%bash" >> $UPD_NB.py && \
    echo "$UPD_EX" >> $UPD_NB.py && \
    echo "#-" >> $UPD_NB.py && \
    py2nb $UPD_NB.py && \
    rm $UPD_NB.py


