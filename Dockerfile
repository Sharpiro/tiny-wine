FROM ubuntu:24.04

ENV CC="clang"
ENV OBJDUMP="objdump"

RUN apt-get update && apt-get -y install \
    build-essential \
    clang \
    binutils \
    less \
    bat \
    vim \
    neofetch

WORKDIR /root/tiny_wine
