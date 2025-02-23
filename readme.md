# Tiny Wine

A basic dynamic loader for Linux and Windows.

## Limitations

- Only works for toy programs created with specific Clang versions
- No recursive dependencies
- `malloc` leaks memory
- `printf` only supports basic formatters

## Prerequisites

All examples use Docker for compatibility, but should work on Linux machines with the necessary packages.
See [Dockerfile](./Dockerfile) for necessary packages.

```sh
git clone https://github.com/Sharpiro/tiny-wine.git
cd tiny-wine
docker build -t tinywine $PWD
```

## Building

```sh
docker run --rm -v $PWD:/root/tiny_wine tinywine make
```

## Running Linux loader

```sh
docker run --rm -v $PWD:/root/tiny_wine tinywine ./loader ./tinyfetch
```

### Example Output

```txt
root@3db29f0a588e
--------------
OS: Ubuntu 22.04.4 LTS armv7l
Kernel: 6.10.5-100.fc39.x86_64
Uptime: Uptime: 32 days, 21 hours, 29 minutes
Shell: /bin/bash
```

## Running Windows loader

```sh
docker run --rm -v $PWD:/root/tiny_wine tinywine ./winloader ./windynamic.exe
```

## Tests

```sh
docker run --rm -v $PWD:/root/tiny_wine tinywine ./test.sh
```

## Tools

Miscellaneous tools that assist with debugging dynamic loaders.

### Readwin

Tool for reading the Windows PE format.

```sh
docker run --rm -v $PWD:/root/tiny_wine tinywine ./readwin ./windynamic.exe
```

## Contributing

This repo is currently experimental and thus not taking contributions at this time.
