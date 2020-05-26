# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import os
import subprocess as sp


def download_file(source, destination):
    print(f"Downloading: {source}")
    command = "wget -nv -O {} {}".format(destination, source)
    status = sp.run(command, shell=True).returncode
    if status != 0:
        raise RuntimeError("Can't load {} to {}.".format(source, destination))


if __name__ == '__main__':

    remote_url = "http://198.74.56.37/ftp/external-data/MD5/"
    target_dir = "data/"

    data_files = {
        "PG3_4871_event.nxs": "a3d0edcb36ab8e9e3342cd8a4440b779",
        "GEM40979.raw": "6df0f1c2fc472af200eec43762e9a874",
        "PG3_4844_event.nxs": "d5ae38871d0a09a28ae01f85d969de1e",
        "PG3_4866_event.nxs": "3d543bc6a646e622b3f4542bc3435e7e"
    }

    for f, h in data_files.items():
        download_file(os.path.join(remote_url, h), os.path.join(target_dir, f))
