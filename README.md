# STM8 Binutils-GDB

Patched GNU Binutils and GDB with support for the STM8 architecture.

## About

This is a fork of the GNU Binutils and GDB toolchain with patches to support the STM8 8-bit microcontroller architecture. The project is based on:

- **Original patches**: [stm8-binutils-gdb on SourceForge](https://sourceforge.net/projects/stm8-binutils-gdb/)
   or [XaviDCR92/stm8-binutils-gdb](https://github.com/XaviDCR92/stm8-binutils-gdb)
- **Updated fork**: [tyalie/stm8-binutils-gdb](https://github.com/tyalie/stm8-binutils-gdb)

This repository includes additional improvements and updates to work with modern GDB versions and OpenOCD.

## Features

- **GDB** with full STM8 support for debugging via OpenOCD
- **Auto-connect and auto-load**: Just use `run` - GDB automatically connects to OpenOCD
- **Binutils** (assembler, linker, objdump, etc.) for STM8
- **Built-in target descriptor** with robust fallback mechanism
- **Colorized disassembly** output for better readability

## Building

### Configure

```bash
./configure \
  --prefix=/usr/local/stow/stm8-binutils-gdb \
  --target=stm8-none-elf32 \
  --program-prefix=stm8-
```

### Build and Install

```bash
make -j$(nproc)
make install
```

### Configuration Options

- `--prefix`: Installation directory (use stow for easy management)
- `--target=stm8-none-elf32`: Target architecture
- `--program-prefix=stm8-`: Prefix for all tools (produces `stm8-gdb`, `stm8-objdump`, etc.)

## Usage

### Debugging with OpenOCD (Simple Method)

1. Start OpenOCD with your STM8 configuration:
   ```bash
   openocd -f interface/stlink.cfg -f target/stm8s.cfg
   ```

2. Launch GDB and just run:
   ```bash
   stm8-gdb your-program.elf
   ```

3. Simply use `run` - GDB will automatically connect to OpenOCD and load your program:
   ```
   (gdb) run
   ```

That's it! No need for manual `target remote` or `load` commands.

### Manual Connection (Alternative)

If you prefer manual control:

```
(gdb) target remote localhost:3333
(gdb) load
(gdb) continue
```

### Disassembly

```bash
stm8-objdump -d your-program.elf
```

## Recent Improvements

- **Auto-connect functionality**: Simply use `run` command - GDB automatically connects to OpenOCD at localhost:3333 and loads the program
- **Robust target descriptor fallback**: GDB validates OpenOCD-supplied target descriptors and falls back to a built-in descriptor if needed
- **Enhanced disassembly**: Colorized output for easier code reading

## Credits

- Original STM8 patches from the [SourceForge project](https://sourceforge.net/projects/stm8-binutils-gdb/)
- Updates and maintenance by [tyalie](https://github.com/tyalie/stm8-binutils-gdb)
- Additional improvements and GDB 15+ compatibility updates

