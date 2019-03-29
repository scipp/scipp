ARG BASE_CONTAINER=jupyter/base-notebook
FROM $BASE_CONTAINER

LABEL maintainer="Igor Gudich <igor.gudich@esss.se>"

USER root

# install needed system packages
RUN apt-get update && \
    apt-get install -y software-properties-common && \
    add-apt-repository ppa:mhier/libboost-latest && \
    apt-get update && \
    apt-get install -y \
    libglu1-mesa \
    cmake \
    g++ \
    mercurial \
    libboost1.67-dev \
    git && \
    rm -rf /var/lib/apt/lists/*


# installing needed conda packages
RUN conda config --add channels conda-forge && \
    conda config --add channels mantid && \
    conda config --add channels mantid/label/nightly && \
    conda config --add channels plotly && \
    conda install --quiet --yes \
    'libglu' \
    'poco' \
    'nexus' \
    'mantid-framework' \
    #installing scipp dependencies
    'xarray' \
    #installing plotly and jupyter extension to visualize scipp
    'plotly=3.6.1' \
    #downgrade tornado for correct work
    'tornado<5.0.0' \
    'jupyterlab=0.35' \
    'ipywidgets>=7.2' \
    'pip'



# enable plotly in notebook
RUN jupyter labextension install @jupyterlab/plotly-extension@0.18.1
#installing tools to convert python scripts to notebooks
RUN pip install \
    py2nb \
    jupyter_contrib_nbextensions \
    jupyter_nbextensions_configurator

RUN jupyter contrib nbextension install && \
    jupyter nbextensions_configurator enable && \
    jupyter nbextension enable exercise2/main


#installing scipp
RUN cd /tmp && \
    git clone https://github.com/mantidproject/scipp.git && \
    cd ./scipp && \
    git submodule init && \
    git submodule update && \
    mkdir /tmp/scipp_build && \
    cd /tmp/scipp_build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/conda/lib/python3.6/site-packages /tmp/scipp && \
    make -j $(lscpu -p | egrep -v '^#' | sort -u -t, -k 2,4 | wc -l)

USER root

RUN cd /tmp/scipp_build && \
    make install

RUN chown -R $NB_USER /tmp/scipp

# Creating scripts for updating and making examples
ARG UPD_EX=/usr/local/bin/update_repo_and_make_examples.sh
ARG EX_DIR=/home/$NB_USER/examples
ARG DEMO_DIR=/home/$NB_USER/demo

RUN touch $UPD_EX && \
    chmod a+x $UPD_EX &&\
    echo "#!/bin/bash" >> $UPD_EX && \
    echo "cd /tmp/scipp && git reset --hard && git clean -fd && git checkout . && git pull" >> $UPD_EX && \
    echo "if [ ! -d $EX_DIR ]; then mkdir $EX_DIR; fi;" >> $UPD_EX && \
    echo "if [ ! -d $DEMO_DIR ]; then mkdir $DEMO_DIR; fi;" >> $UPD_EX && \
    echo "if [ -d /tmp/scipp/scippy/demo ] && [ -d $DEMO_DIR ]; then cp -r /tmp/scipp/scippy/demo/* $DEMO_DIR ; fi;" >> $UPD_EX && \
    echo "find /tmp/scipp/scippy/examples -name '*.py' -exec py2nb {} \; ;" >> $UPD_EX && \
    echo "if [ -d /tmp/scipp/scippy/examples ] && [ -d $EX_DIR ]; then find /tmp/scipp/scippy/examples -name '*.ipynb' -exec mv {} $EX_DIR \; ; fi;" >> $UPD_EX && \
    echo "exit 0" >> $UPD_EX

# Avoid using passwords for jupyter notebook
RUN sed -i "s/jupyter notebook/jupyter notebook --NotebookApp.token='' --NotebookApp.password=''/" /usr/local/bin/start-notebook.sh

# Add datafiles needed for demo
ARG PG3_4844_HASH=d5ae38871d0a09a28ae01f85d969de1e
ARG PG3_4866_HASH=3d543bc6a646e622b3f4542bc3435e7e

RUN mkdir $DEMO_DIR && \
    curl http://198.74.56.37/ftp/external-data/MD5/$PG3_4844_HASH --output $DEMO_DIR/PG3_4844_event.nxs && \
    curl http://198.74.56.37/ftp/external-data/MD5/$PG3_4866_HASH --output $DEMO_DIR/PG3_4866_event.nxs && \
    chown -R $NB_USER /home/$NB_USER/work

RUN fix-permissions /home/$NB_USER

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

RUN rm -r /home/$NB_USER/work

