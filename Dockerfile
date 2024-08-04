FROM ubuntu:jammy

ENV QEMU_LD_PREFIX="/usr/arm-linux-gnueabihf"
ENV PATH="/root/zig-linux-x86_64-0.13.0:${PATH}"

RUN apt-get update && apt-get -y install \
    build-essential \
    curl \
    clang \
    gcc-arm-linux-gnueabihf \
    qemu-user

WORKDIR /root

RUN curl -fLO https://ziglang.org/download/0.13.0/zig-linux-x86_64-0.13.0.tar.xz
RUN tar -vxaf zig-linux-x86_64-0.13.0.tar.xz

WORKDIR /root/tiny_wine

# RUN zig cc --target=arm-linux-gnueabihf temp.c
# ENTRYPOINT [ "zig", "cc", "--target=arm-linux-gnueabihf"]
# CMD [ "zig", "cc", "--target=arm-linux-gnueabihf"]
# ENV CC="zig cc --target=arm-linux-gnueabihf"
CMD [ "make", "tiny_c"]
# qemu-arm ./a.out
