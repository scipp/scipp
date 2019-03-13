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
    mkdir build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/conda/lib/python3.6/site-packages .. && \
    make -j $(lscpu -p | egrep -v '^#' | sort -u -t, -k 2,4 | wc -l)

USER root

RUN cd /tmp/dataset/build && \
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

ARG UPD_EX=/usr/local/bin/update_examples.sh

RUN chmod -R a+rwx /home/$NB_USER/work && \
    chmod -R a+rwx /tmp/dataset

RUN touch $UPD_EX && \
    chmod a+x $UPD_EX &&\
    echo "#!/bin/bash" >> $UPD_EX && \
    echo "cd /tmp/dataset && git pull" >> $UPD_EX && \
    echo "find /tmp/dataset/python/examples -name '*.py' -exec py2nb {} \; ;" >> $UPD_EX && \
    echo "if [ ! -d /home/$NB_USER/work/examples ]; then mkdir /home/$NB_USER/work/examples; fi;" >> $UPD_EX && \
    echo  "if [ -d /tmp/dataset/python/examples ] && [ -d /home/$NB_USER/work/examples ]; then find /tmp/dataset/python/examples -name '*.ipynb' -exec mv {} /home/$NB_USER/work/examples \; ; fi;" >> $UPD_EX && \
    touch /home/$NB_USER/thefile.txt && echo $(date) >> /home/$NB_USER/thefile.txt

RUN sed -i "5s/^/exec update_examples.sh\n\n/" /usr/local/bin/start-notebook.sh

USER $NB_UID



