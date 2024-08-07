FROM ubuntu:jammy

ENV QEMU_LD_PREFIX="/usr/arm-linux-gnueabihf"
ENV CC="clang"
ENV CFLAGS="--target=arm-linux-gnueabihf --sysroot=/root/sysroot"
ENV OBJDUMP="/usr/arm-linux-gnueabihf/bin/objdump"

RUN apt-get update && apt-get -y install \
    build-essential \
    curl \
    clang \
    gcc-arm-linux-gnueabihf \
    qemu-user \
    debootstrap \
    file

RUN mkdir -p /root/sysroot
RUN \
    debootstrap --arch=armhf --foreign bullseye /root/sysroot http://deb.debian.org/debian && \
    chroot /root/sysroot /debootstrap/debootstrap --second-stage

WORKDIR /root/tiny_wine
