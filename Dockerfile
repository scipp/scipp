FROM jupyter/base-notebook

# Avoid using passwords for jupyter notebook
USER root
RUN sed -i "s/jupyter notebook/jupyter notebook --NotebookApp.token='' --NotebookApp.password=''/" /usr/local/bin/start-notebook.sh
USER $NB_USER

# Remove default "work" directory
RUN rm -r "/home/$NB_USER/work"

# Enable Plotly JupyterLab extension
RUN jupyter labextension install @jupyterlab/plotly-extension@0.18.1

# Install Scipp
RUN conda install --yes -c scipp/label/dev scipp

# Add the demo notebooks and Python examples
ADD 'python/demo/' "/home/$NB_USER/demo"
ADD 'python/examples/' "/home/$NB_USER/examples"
USER root
RUN chown -R "$NB_USER" \
      "/home/$NB_USER/demo" \
      "/home/$NB_USER/examples"
USER $NB_USER
