FROM jupyter/base-notebook

# Avoid using passwords for jupyter notebook
USER root
RUN sed -i "s/jupyter notebook/jupyter notebook --NotebookApp.token='' --NotebookApp.password=''/" /usr/local/bin/start-notebook.sh
USER $NB_USER

# Remove default "work" directory
RUN rm -r "/home/$NB_USER/work"

# Enable Plotly JupyterLab extension
RUN jupyter labextension install @jupyterlab/plotly-extension@0.18.1

# Install Scipp and dependencies
RUN conda install --yes \
      -c scipp/label/dev \
      -c mantid \
      ipython \
      matplotlib \
      plotly \
      scipp \
      ipywidgets \
      mantid-framework \
      python=3.6

# Add datafiles needed for demo
ARG PG3_4844_HASH=d5ae38871d0a09a28ae01f85d969de1e
ARG PG3_4866_HASH=3d543bc6a646e622b3f4542bc3435e7e

RUN mkdir $DEMO_DIR && \
    curl http://198.74.56.37/ftp/external-data/MD5/$PG3_4844_HASH --output $DEMO_DIR/PG3_4844_event.nxs && \
    curl http://198.74.56.37/ftp/external-data/MD5/$PG3_4866_HASH --output $DEMO_DIR/PG3_4866_event.nxs && \
    chown -R $NB_USER /home/$NB_USER/work

# Add the tutorials and user guide notebooks
ADD 'python/demo/' "/home/$NB_USER/demo"
ADD 'docs/tutorials/' "/home/$NB_USER/tutorials"
ADD 'docs/user-guide/' "/home/$NB_USER/user-guide"
USER root
RUN chown -R "$NB_USER" \
      "/home/$NB_USER/demo" \
      "/home/$NB_USER/tutorials" \
      "/home/$NB_USER/work" \
      "/home/$NB_USER/user-guide"
USER $NB_USER
