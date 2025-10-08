import site
from pathlib import Path


def patch_conan() -> bool:
    for site_dir in site.getsitepackages():
        conf_file = Path(site_dir) / "conans/client/conf/__init__.py"
        if conf_file.exists():
            break
    else:
        print("No Conan config file found, skipping")
        return False
    content = conf_file.read_text()
    old_version_string = """apple-clang: &apple_clang
            version: ["5.0", "5.1", "6.0", "6.1", "7.0", "7.3", "8.0", "8.1", "9.0", "9.1",
                      "10.0", "11.0", "12.0", "13", "13.0", "13.1", "14", "14.0", "15", "15.0"]"""  # noqa: E501
    new_version_string = """apple-clang: &apple_clang
            version: ["17", "17.0"]"""
    content = content.replace(old_version_string, new_version_string)
    conf_file.write_text(content)
    print(f"Patched Conan config file {conf_file}")
    return True


if __name__ == "__main__":
    patch_conan()
