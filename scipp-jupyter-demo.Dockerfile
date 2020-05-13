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

# Add datafiles needed for neutron tutorials
ARG FTPURL="http://198.74.56.37/ftp/external-data/MD5/"
ARG PG3_4844_HASH=d5ae38871d0a09a28ae01f85d969de1e
ARG PG3_4866_HASH=3d543bc6a646e622b3f4542bc3435e7e
ARG PG3_4871_HASH=a3d0edcb36ab8e9e3342cd8a4440b779
ARG GEM40979_HASH=6df0f1c2fc472af200eec43762e9a874
RUN wget --quiet -O "/home/$NB_USER/data/PG3_4844_event.nxs" "${FTPURL}${PG3_4844_HASH}" && \
    wget --quiet -O "/home/$NB_USER/data/PG3_4866_event.nxs" "${FTPURL}${PG3_4866_HASH}" && \
    wget --quiet -O "/home/$NB_USER/data/PG3_4871_event.nxs" "${FTPURL}${PG3_4871_HASH}" && \
    wget --quiet -O "/home/$NB_USER/data/GEM40979.raw" "${FTPURL}${GEM40979_HASH}"

# Install Scipp and dependencies
RUN conda install --yes \
      -c conda-forge \
      -c scipp/label/dev \
      -c dannixon \
      ipyevents \
      ipython \
      ipyvolume \
      ipywidgets \
      mantid-framework \
      matplotlib \
      python=3.7 \
      scipp && \
    conda clean -afy

# Avoid weird tornado AttributeError
RUN pip install --upgrade nbconvert

# Add the tutorials and user guide notebooks
ADD 'docs/event-data/' "/home/$NB_USER/event-data"
ADD 'docs/scipp-neutron/' "/home/$NB_USER/scipp-neutron"
ADD 'docs/tutorials/' "/home/$NB_USER/tutorials"
ADD 'docs/user-guide/' "/home/$NB_USER/user-guide"
ADD 'docs/visualization/' "/home/$NB_USER/visualization"
USER root
RUN chown -R "$NB_USER" \
      "/home/$NB_USER/event-data" \
      "/home/$NB_USER/scipp-neutron" \
      "/home/$NB_USER/tutorials" \
      "/home/$NB_USER/user-guide" \
      "/home/$NB_USER/visualization"
USER $NB_USER
