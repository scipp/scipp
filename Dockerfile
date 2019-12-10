FROM jupyter/base-notebook

# Avoid using passwords for jupyter notebook
USER root
RUN apt-get update
RUN apt-get install -y libgl1 libglu1-mesa && \
    apt-get clean
RUN sed -i "s/jupyter notebook/jupyter notebook --NotebookApp.token='' --NotebookApp.password=''/" /usr/local/bin/start-notebook.sh

USER $NB_USER

# Remove default "work" directory
RUN rm -r "/home/$NB_USER/work"
RUN mkdir -p "/home/$NB_USER/data"

# Add datafiles needed for neutron tutorial
ARG PG3_4844_HASH=d5ae38871d0a09a28ae01f85d969de1e
ARG PG3_4866_HASH=3d543bc6a646e622b3f4542bc3435e7e
RUN wget -O /home/$NB_USER/data/PG3_4844_event.nxs http://198.74.56.37/ftp/external-data/MD5/$PG3_4844_HASH && \
    wget -O /home/$NB_USER/data/PG3_4866_event.nxs http://198.74.56.37/ftp/external-data/MD5/$PG3_4866_HASH

# Install Scipp and dependencies
RUN conda install --yes \
      -c conda-forge \
      -c scipp/label/dev \
      -c mantid/label/nightly \
      -c mantid \
      ipython \
      matplotlib \
      scipp \
      ipywidgets \
      ipyvolume \
      ipyevents \
      mantid-framework=4 \
      python=3.6 \
      numpy=1.15 && \
    conda clean -afy

# Avoid weird tornado AttributeError
RUN pip install --upgrade nbconvert

# Add the tutorials and user guide notebooks
ADD 'python/demo/' "/home/$NB_USER/demo"
ADD 'docs/tutorials/' "/home/$NB_USER/tutorials"
ADD 'docs/user-guide/' "/home/$NB_USER/user-guide"
USER root
RUN chown -R "$NB_USER" \
      "/home/$NB_USER/demo" \
      "/home/$NB_USER/tutorials" \
      "/home/$NB_USER/user-guide"
USER $NB_USER
