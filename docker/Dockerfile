FROM jupyter/base-notebook

# Install Scipp
RUN conda install --yes -c scipp/label/dev scipp

# Enable Plotly JupyterLab extension
RUN jupyter labextension install @jupyterlab/plotly-extension@0.18.1

# Avoid using passwords for jupyter notebook
USER root
RUN sed -i "s/jupyter notebook/jupyter notebook --NotebookApp.token='' --NotebookApp.password=''/" /usr/local/bin/start-notebook.sh
USER $NB_USER

RUN rm -r /home/$NB_USER/work
