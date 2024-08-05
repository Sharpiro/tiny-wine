FROM ubuntu:jammy
# FROM debian:bullseye

ENV QEMU_LD_PREFIX="/usr/arm-linux-gnueabihf"
ENV PATH="/root/zig-linux-x86_64-0.13.0:${PATH}"

RUN apt-get update && apt-get -y install \
    build-essential \
    curl \
    clang \
    gcc-arm-linux-gnueabihf \
    qemu-user

# RUN apt-get update && apt-get -y install qemu-user

WORKDIR /root

ENV CC="clang"

RUN apt-get -y install debootstrap file

RUN mkdir -p /root/sysroot

RUN debootstrap --arch=armhf --foreign bullseye /root/sysroot http://deb.debian.org/debian
RUN chroot /root/sysroot /debootstrap/debootstrap --second-stage

WORKDIR /root/tiny_wine

ENV CFLAGS="--target=arm-linux-gnueabihf --sysroot=/root/sysroot"
ENV OBJDUMP="/usr/arm-linux-gnueabihf/bin/objdump"

# RUN echo $CHROOT

# RUN zig cc --target=arm-linux-gnueabihf temp.c
# ENTRYPOINT [ "zig", "cc", "--target=arm-linux-gnueabihf"]
# CMD [ "zig", "cc", "--target=arm-linux-gnueabihf"]
# ENV CC="zig cc --target=arm-linux-gnueabihf"
# CMD [ "make" ]
# qemu-arm ./a.out

# clang --target=arm-linux-gnueabihf --sysroot=/root/sysroot temp.c
# make tiny_c
