import doctest
import os
import sys

import scipp

sys.path.insert(0, os.path.abspath('.'))

# General information about the project.
project = 'Scipp'
copyright = '2024 Scipp contributors'
author = 'Scipp contributors'

html_show_sourcelink = True

# -- General configuration ------------------------------------------------

# If your documentation needs a minimal Sphinx version, state it here.
#
# needs_sphinx = '1.0'

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.autosummary',
    'sphinx.ext.doctest',
    'sphinx.ext.intersphinx',
    'sphinx.ext.mathjax',
    'sphinx.ext.napoleon',
    'sphinx_autodoc_typehints',
    'sphinx_copybutton',
    'IPython.sphinxext.ipython_directive',
    'IPython.sphinxext.ipython_console_highlighting',
    'matplotlib.sphinxext.plot_directive',
    'nbsphinx',
    'scipp.sphinxext.autoplot',
    'myst_parser',
]

myst_enable_extensions = [
    "amsmath",
    "colon_fence",
    "deflist",
    "dollarmath",
    "fieldlist",
    "html_admonition",
    "html_image",
    "replacements",
    "smartquotes",
    "strikethrough",
    "substitution",
    "tasklist",
]

myst_heading_anchors = 3

autodoc_type_aliases = {
    'DTypeLike': 'DTypeLike',
    'VariableLike': 'VariableLike',
    'MetaDataMap': 'MetaDataMap',
    'array_like': 'array_like',
}

rst_epilog = f"""
.. |SCIPP_RELEASE_MONTH| replace:: {os.popen("git show -s --format=%cd --date=format:'%B %Y'").read()}
.. |SCIPP_VERSION| replace:: {os.popen("git describe --tags --abbrev=0").read()}
"""  # noqa: E501, S605, S607

intersphinx_mapping = {
    'python': ('https://docs.python.org/3', None),
    'numpy': ('https://numpy.org/doc/stable/', None),
    'pandas': ('https://pandas.pydata.org/docs/', None),
    'scipy': ('https://docs.scipy.org/doc/scipy/', None),
    'xarray': ('https://xarray.pydata.org/en/stable/', None),
}

# autodocs includes everything, even irrelevant API internals. autosummary
# looks more suitable in the long run when the API grows.
# For a nice example see how xarray handles its API documentation.
autosummary_generate = True

napoleon_google_docstring = False
napoleon_numpy_docstring = True
napoleon_use_param = True
napoleon_use_rtype = False
napoleon_preprocess_types = True
napoleon_type_aliases = {
    # objects without namespace: scipp
    "DataArray": "~scipp.DataArray",
    "Dataset": "~scipp.Dataset",
    "Variable": "~scipp.Variable",
    # objects without namespace: numpy
    "ndarray": "~numpy.ndarray",
}
typehints_defaults = 'comma'
typehints_use_rtype = False

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# The suffix(es) of source filenames.
# You can specify multiple suffix as a list of string:
#
source_suffix = ['.rst', '.md']
html_sourcelink_suffix = ''  # Avoid .ipynb.txt extensions in sources

# The master toctree document.
master_doc = 'index'

# The version info for the project you're documenting, acts as replacement for
# |version| and |release|, also used in various other places throughout the
# built documents.
#
# The short X.Y version.
version = scipp.__version__
# The full version, including alpha/beta/rc tags.
release = scipp.__version__

warning_is_error = True

# The language for content autogenerated by Sphinx. Refer to documentation
# for a list of supported languages.
#
# This is also used if you do content translation via gettext catalogs.
# Usually you set "language" from the command line for these cases.
language = 'en'

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This patterns also effect to html_static_path and html_extra_path
exclude_patterns = [
    '_build',
    'Thumbs.db',
    '.DS_Store',
    '**.ipynb_checkpoints',
    'development/project',
]

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = 'sphinx'

# If true, `todo` and `todoList` produce output, else they produce nothing.
todo_include_todos = False

# -- Options for HTML output ----------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#

html_theme = "pydata_sphinx_theme"
html_theme_options = {
    "primary_sidebar_end": ["edit-this-page", "sourcelink"],
    "secondary_sidebar_items": [],
    "navbar_persistent": ["search-button"],
    "show_nav_level": 1,
    # Adjust this to ensure external links are moved to "Move" menu
    "header_links_before_dropdown": 5,
    "pygment_light_style": "github-light-high-contrast",
    "pygment_dark_style": "github-dark-high-contrast",
    "logo": {
        "image_light": "_static/logo-2022.svg",
        "image_dark": "_static/logo-dark.svg",
    },
    "external_links": [
        {"name": "Plopp", "url": "https://scipp.github.io/plopp"},
        {"name": "Scippnexus", "url": "https://scipp.github.io/scippnexus"},
        {"name": "Scippneutron", "url": "https://scipp.github.io/scippneutron"},
        {"name": "Ess", "url": "https://scipp.github.io/ess"},
    ],
    "icon_links": [
        {
            "name": "GitHub",
            "url": "https://github.com/scipp/scipp",
            "icon": "fa-brands fa-github",
            "type": "fontawesome",
        },
        {
            "name": "PyPI",
            "url": "https://pypi.org/project/scipp/",
            "icon": "fa-brands fa-python",
            "type": "fontawesome",
        },
        {
            "name": "Conda",
            "url": "https://anaconda.org/scipp/scipp",
            "icon": "fa-custom fa-anaconda",
            "type": "fontawesome",
        },
    ],
    "footer_start": ["copyright", "sphinx-version"],
    "footer_end": ["doc_version", "theme-version"],
}
html_context = {
    "doc_path": "docs",
}
html_sidebars = {
    "**": ["sidebar-nav-bs", "page-toc"],
}

html_title = "Scipp"
html_logo = "_static/logo-2022.svg"
html_favicon = "_static/favicon.ico"

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']
html_css_files = []
html_js_files = ["anaconda-icon.js"]
# -- Options for HTMLHelp output ------------------------------------------

# Output file base name for HTML help builder.
htmlhelp_basename = 'scippdoc'

# -- Options for Matplotlib in notebooks ----------------------------------

nbsphinx_execute_arguments = [
    "--Session.metadata=scipp_sphinx_build=True",
]

# -- Options for matplotlib in docstrings ---------------------------------

plot_include_source = True
plot_formats = ['png']
plot_html_show_formats = False
plot_html_show_source_link = False
plot_pre_code = '''import scipp as sc'''

# -- Options for doctest --------------------------------------------------

# sc.plot returns a Figure object and doctest compares that against the
# output written in the docstring. But we only want to show an image of the
# figure, not its `repr`.
# In addition, there is no need to make plots in doctest as the documentation
# build already tests if those plots can be made.
# So we simply disable plots in doctests.
doctest_global_setup = '''
import numpy as np
import scipp as sc

def do_not_plot(*args, **kwargs):
    pass

sc.plot = do_not_plot
sc.Variable.plot = do_not_plot
sc.DataArray.plot = do_not_plot
sc.Dataset.plot = do_not_plot
'''

# Using normalize whitespace because many __str__ functions in Scipp produce
# extraneous empty lines and it would look strange to include them in the docs.
doctest_default_flags = (
    doctest.ELLIPSIS
    | doctest.IGNORE_EXCEPTION_DETAIL
    | doctest.DONT_ACCEPT_TRUE_FOR_1
    | doctest.NORMALIZE_WHITESPACE
)

# -- Options for linkcheck ------------------------------------------------

linkcheck_ignore = [
    # Specific lines in Github blobs cannot be found by linkcheck.
    r'https?://github\.com/.*?/blob/[a-f0-9]+/.+?#',
    # Many links for PRs from our release notes. Slow and unlikely to cause issues.
    'https://github.com/scipp/scipp/pull/[0-9]+',
    # NASA mission website for solar flares data is down
    'https://hesperia.gsfc.nasa.gov/rhessi3+',
    # Flaky Kitware repo link
    'https://apt.kitware.com+',
    # Flaky pooch fatiando website
    'https://www.fatiando.org/pooch+',
    # Fails to validate SSL certificate but Firefox is happy with the URL.
    'https://physics.nist.gov/cuu/Constants/',
    # This link works but is very slow and causes a timeout.
    'https://anaconda.org/conda-forge',
    # Linkcheck seems to be denied access by some DOI resolvers.
    # Since DOIs are supposed to be permanent, we don't need to check them.'
    r'https://doi\.org/',
    # Website denies access to linkcheck.
    'https://content.iospress.com/articles/journal-of-neutron-research/jnr190131',
]
