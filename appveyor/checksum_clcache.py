import os
import checksumdir


def stamp_cache():
    """ Stamp the cache directory root with a file as a comparitor.
    At time of writing, not looks like appveyor does not properly
    compare clcache directory between builds.

    File uniquely created based on checksum of all files in directory
    enabling appveyor to indentify changed or identical caches
    """
    cache_dir = os.path.join(os.path.expanduser('~'), 'clcache')
    check_sum = checksumdir.dirhash(cache_dir, excluded_extensions=['hash'])
    # Write empty file with check_sum as name
    f = open(os.path.join(cache_dir, '{}.hash'.format(check_sum)), 'w')
    f.close()
    # Diagnostic
    print('cache hash', check_sum)


def main():
    stamp_cache()


if __name__ == "__main__":
    main()
