import pytest
import scipp as sc

# Our config API does not expose a way to ignore user configuration files.
# So we cannot test loading of the default file because a user's config file
# takes precedence and can break a test.


@pytest.fixture
def working_dir(request, monkeypatch):
    # Change working directory to the directory the test case is in.
    monkeypatch.chdir(request.fspath.dirname)


@pytest.fixture
def clean_config():
    sc.config.get.cache_clear()
    yield
    sc.config.get.cache_clear()


def test_loads_local(working_dir, clean_config):
    assert sc.config['table_max_size'] == 1
    assert sc.config['colors']['attrs'] == '#123456'
