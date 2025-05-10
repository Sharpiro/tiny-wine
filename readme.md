# Tiny Wine

A basic dynamic loader for Linux and Windows.

## Limitations

- Only works for toy programs created with MinGW and specific Clang versions in this repository
- x64 only
- No recursive dependencies
- `malloc` leaks memory
- loaders use a lot of memory
- Import Address Table limit is 512 entries
- Windows header size limit is 4 KiB
- No environment variables
- No AVX
- No Printing floats
- dlls must be in current working directory

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
docker run --rm -v $PWD:/home/tiny_wine tinywine make
```

## Running Linux loader

```sh
docker run --rm -v $PWD:/home/tiny_wine -w /home/tiny_wine/build tinywine ./linloader ./tinyfetch
```

### Example Output

```txt
tiny_wine@54510a2c9be4
--------------
OS: Ubuntu 22.04.5 LTS x86_64
Kernel: 6.11.9-100.fc39.x86_64
Uptime: 46 days, 18 hours, 52 minutes
Shell: /bin/sh
```

## Running Windows loader

```sh
docker run --rm -v $PWD:/home/tiny_wine -w /home/tiny_wine/build tinywine ./winloader ./windynamic.exe
```

## Tests

```sh
docker run --rm -v $PWD:/home/tiny_wine tinywine ./test.sh
```

## Tools

Miscellaneous tools that assist with debugging dynamic loaders.

### Readwin

Tool for reading the Windows PE format.

```sh
docker run --rm -v $PWD:/home/tiny_wine -w /home/tiny_wine/build tinywine ./readwin ./windynamic.exe
```

## Contributing

This repo is currently experimental and thus not taking contributions at this time.
