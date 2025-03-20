FROM ubuntu:22.04

ENV CC="clang"
ENV OBJDUMP="objdump"

RUN apt-get update && apt-get -y install \
    build-essential \
    clang \
    binutils \
    less \
    bat \
    vim \
    neofetch \ 
    mingw-w64 \
    mingw-w64-tools \
    binutils-mingw-w64

RUN useradd -m tiny_wine
USER tiny_wine

WORKDIR /home/tiny_wine
