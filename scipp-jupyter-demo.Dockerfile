FROM jupyter/base-notebook

# Avoid using passwords for jupyter notebook
USER root
RUN apt-get update && \
    apt-get install -y libgl1 libglu1-mesa git && \
    apt-get clean
RUN sed -i "s/jupyter notebook/jupyter notebook --NotebookApp.token='' --NotebookApp.password=''/" /usr/local/bin/start-notebook.sh

USER $NB_USER

# Remove default "work" directory
RUN rm -r "/home/$NB_USER/work"
RUN mkdir -p "/home/$NB_USER/code"
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
# Datafiles for SANS direct-beam iteration demo
ARG SANSURL="https://github.com/ess-dmsc-dram/loki_tube_scripts/raw/master/test/test_data/"
RUN wget --quiet -P "/home/$NB_USER/data" "${SANSURL}LARMOR00049334.nxs" && \
    wget --quiet -P "/home/$NB_USER/data" "${SANSURL}LARMOR00049335.nxs" && \
    wget --quiet -P "/home/$NB_USER/data" "${SANSURL}LARMOR00049338.nxs" && \
    wget --quiet -P "/home/$NB_USER/data" "${SANSURL}LARMOR00049339.nxs" && \
    wget --quiet -P "/home/$NB_USER/data" "${SANSURL}DirectBeam_20feb_full_v3.dat" && \
    wget --quiet -P "/home/$NB_USER/data" "${SANSURL}ModeratorStdDev_TS2_SANS_LETexptl_07Aug2015.txt"


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

# Get code for SANS direct-beam iteration demo
RUN cd "/home/$NB_USER/code" && \
    git clone https://github.com/scipp/ess.git && \
    cd ess/sans && \
    python make_config.py -f "/home/$NB_USER/data" && \
    ln -s "/home/$NB_USER/code/ess/sans" "/home/$NB_USER/sans-demo"

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
