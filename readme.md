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
docker run --rm -v $PWD:/root/tiny_wine tinywine qemu-arm ./loader ./tinyfetch
```

### Example Output

```txt
root@3db29f0a588e
--------------
OS: Ubuntu 22.04.4 LTS armv7l
Kernel: 6.10.5-100.fc39.x86_64
Uptime: 0x00000024 days, 0x00000016 hours, 0x00000033 minutes
Shell: (null)
```

## Tests

```sh
docker run --rm -v $PWD:/root/tiny_wine tinywine ./test.sh
```

## Contributing

This repo is currently experimental and thus not taking contributions at this time.
