from pathlib import Path

# The name of the C++ module that contains the classes we generate stubs for.
CPP_CORE_MODULE_NAME = 'scipp._scipp.core'

PY_CORE_MODULE_NAME = 'scipp.core'

TEMPLATE_FILE = Path(__file__).resolve().parent / "stub_template.py.template"


def class_is_excluded(name: str) -> bool:
    return name.startswith('ElementArrayView') or name.startswith('_')
