FROM ubuntu:jammy

ENV QEMU_LD_PREFIX="/usr/arm-linux-gnueabihf"
ENV CC="clang"
ENV CFLAGS="--target=arm-linux-gnueabihf"
ENV OBJDUMP="/usr/arm-linux-gnueabihf/bin/objdump"
ENV PRELOADER="qemu-arm"

RUN apt-get update && apt-get -y install \
    build-essential \
    clang \
    gcc-arm-linux-gnueabihf \
    qemu-user

WORKDIR /root/tiny_wine
