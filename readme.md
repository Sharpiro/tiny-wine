# Tiny Wine

A Project to help understand how linkers and Wine work.

Very basic support for jumping to other static Linux ELF programs.

## Limitations

- Arm32
- Linux
- No standard libary

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
docker run --rm -v $PWD:/root/tiny_wine tinywine qemu-arm ./loader ./env always be closing
```

## Tests

```sh
docker run --rm -v $PWD:/root/tiny_wine tinywine ./test.sh
```

## Contributing

This repo is currently experimental and thus not taking contributions at this time.
