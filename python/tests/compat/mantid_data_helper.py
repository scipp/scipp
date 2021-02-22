# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Dimitar Tasev

import os
import hashlib
import subprocess as sp


def mantid_is_available():
    try:
        import mantid  # noqa: F401
        return True
    except ImportError:
        return False


def download_file(source, destination):
    command = "wget -O {} {}".format(destination, source)
    status = sp.run(command, shell=True).returncode
    if status != 0:
        raise RuntimeError("Can't load {} to {}.".format(source, destination))


class MantidDataHelper:
    # Valid only for Linux. Windows is as C:\MantidExternalData
    DATA_DIR = os.path.abspath(os.path.expanduser("~/MantidExternalData"))
    DATA_LOCATION = "{data_dir}/{algorithm}/{hash}"
    DATA_FILES = {
        "CNCS_51936_event.nxs": {
            "hash": "5ba401e489260a44374b5be12b780911",
            "algorithm": "MD5"
        },
        "iris26176_graphite002_sqw.nxs": {
            "hash": "7ea63f9137602b7e9b604fe30f0c6ec2",
            "algorithm": "MD5"
        },
        "WISH00016748.raw": {
            "hash": "37ecc6f99662b57e405ed967bdc068af",
            "algorithm": "MD5"
        },
    }
    REMOTE_URL = "http://198.74.56.37/ftp/external-data/"\
        "{algorithm}/{hash}"

    @classmethod
    def find_file(cls, hash, algorithm):
        data_location = cls.DATA_LOCATION.format(data_dir=cls.DATA_DIR,
                                                 algorithm=algorithm,
                                                 hash=hash)
        dir_name = os.path.dirname(data_location)
        if not os.path.exists(dir_name):
            os.makedirs(dir_name, exist_ok=True)

        if not os.path.isfile(data_location):
            query = cls.REMOTE_URL.format(algorithm=algorithm, hash=hash)
            download_file(query, data_location)
            if algorithm == "MD5":
                with open(data_location, "rb") as file:
                    md5 = hashlib.md5(file.read()).hexdigest()
                    if md5 != hash:
                        raise RuntimeError("Check sum doesn't match.")

        return data_location

    @classmethod
    def find_known_file(cls, name):
        data_file = cls.DATA_FILES[name]
        return cls.find_file(data_file["hash"], data_file["algorithm"])
