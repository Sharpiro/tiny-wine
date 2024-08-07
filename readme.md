# Tiny Wine

Educational repo to help me understand how linkers and Wine works.

Very basic support for jumping to other static Linux ELF programs.

## Limitations (A lot)

- Arm32 only
- Linux Elf only (yes, no windows support despite being called 'Tiny Wine')
- Statically linked only
- Can only do basic things found in "tiny_c"

## Prerequisites

- docker (or an arm32 machine)

```sh
git clone https://github.com/Sharpiro/tiny-wine.git
cd tiny-wine
docker build -t tinywine $PWD
```

## Building

```sh
docker run --rm -v $PWD:/root/tiny_wine tinywine make
```

## Running

```sh
docker run --rm -v $PWD:/root/tiny_wine tinywine ./loader ./env always be closing
```

## Tests

```sh
docker run --rm -v $PWD:/root/tiny_wine tinywine ./test.sh
```
