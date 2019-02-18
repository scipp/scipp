ARG BASE_CONTAINER=jupyter/minimal-notebook
FROM $BASE_CONTAINER

LABEL maintainer="Igor Gudich <igor.gudich@esss.se>"

USER root

# cmake and clang to compile source code
RUN apt-get update && \
    apt-get install -y cmake && \
    apt-get install -y clang-6.0 && \
    apt-get install -y mercurial && \
    apt-get install -y libboost-all-dev && \

    rm -rf /var/lib/apt/lists/*

USER $NB_UID

# Install Python 3 packages
# Remove pyqt and qt pulled in for matplotlib since we're only ever going to
# use notebook-friendly backends in these images
RUN conda config --add channels pyviz && \
    conda install --quiet --yes \
    'conda-forge::blas=*=openblas' \
    'ipywidgets=7.4*' \
    'matplotlib=2.2*' \
    'bokeh=0.13*' \
    'hdf5=1.10*' \
    'h5py=2.7*' \
    'hvplot' \
    'holoplot' \
    'pyviz' \ 
    'holoviews' \
    'xarray' \
    'xlrd'  && \
    conda remove --quiet --yes --force qt pyqt && \
    conda clean -tipsy && \
    # Activate ipywidgets extension in the environment that runs the notebook server
    jupyter nbextension enable --py widgetsnbextension --sys-prefix && \
    # Also activate ipywidgets extension for JupyterLab
    # Check this URL for most recent compatibilities
    # https://github.com/jupyter-widgets/ipywidgets/tree/master/packages/jupyterlab-manager
    jupyter labextension install @jupyter-widgets/jupyterlab-manager@^0.38.1 && \
    jupyter labextension install jupyterlab_bokeh@0.6.3 && \
    jupyter labextension install @pyviz/jupyterlab_pyviz && \
    npm cache clean --force && \
    rm -rf $CONDA_DIR/share/jupyter/lab/staging && \
    rm -rf /home/$NB_USER/.cache/yarn && \
    rm -rf /home/$NB_USER/.node-gyp && \
    fix-permissions $CONDA_DIR && \
    fix-permissions /home/$NB_USER

RUN cd /tmp && \
    git clone https://github.com/mantidproject/dataset.git && \
    cd ./dataset && \
    git submodule init && \
    git submodule update && \
    mkdir build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/conda/lib/python3.6/site-packages .. && \
    make -j 12

USER root

RUN cd /tmp/dataset/build && \
    make install 

RUN sudo apt-get purge --auto-remove -y libboost-all-dev 
   
USER $NB_UID

