
import os
import urllib


class MantidDataHelper:
    # Valid only for Linux. Windows is as C:\MantidExternalData
    DATA_DIR = os.path.abspath(os.path.expanduser("~/MantidExternalData"))
    DATA_LOCATION = "{data_dir}/{algorithm}/{hash}"
    DATA_FILES = {
        "CNCS_51936_event.nxs": {
            "hash": "5ba401e489260a44374b5be12b780911",
            "algorithm": "MD5"}
    }
    REMOTE_URL = "http://mantidweb.nd.rl.ac.uk/externaldata/isis-readonly/"\
        "{algorithm}/{hash}"

    @classmethod
    def find_file(cls, name):
        data_file = cls.DATA_FILES[name]

        data_location = cls.DATA_LOCATION.format(
            data_dir=cls.DATA_DIR,
            algorithm=data_file["algorithm"],
            hash=data_file["hash"])

        if not os.path.isfile(data_location):
            data_location, http_response = urllib.request.urlretrieve(
                cls.REMOTE_URL.format(algorithm=data_file["algorithm"],
                                      hash=data_file["hash"]), data_location)

        return data_location
