FROM archlinux

RUN pacman -Sy --noconfirm cppcheck && \
    mkdir -p '/data/'

VOLUME ['/data']
CMD ["cppcheck", "--quiet", "--force", "--std=c++17", "--enable=all", "-i", "/data/common/include/scipp/common/span/", "/data"]
