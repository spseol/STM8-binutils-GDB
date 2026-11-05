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

## Installation

### Debian-like distributions (APT repository)

A prebuilt package for Debian-like distributions (amd64) is available from
the APT repository at <https://mamut.spseol.cz/debian/>:

```bash
sudo curl -fsSL https://mamut.spseol.cz/debian/mamut-archive-keyring.gpg -o /etc/apt/keyrings/mamut-archive-keyring.gpg
echo "deb [arch=amd64 signed-by=/etc/apt/keyrings/mamut-archive-keyring.gpg] https://mamut.spseol.cz/debian mamut main" | sudo tee /etc/apt/sources.list.d/mamut.list
sudo apt update
sudo apt install stm8-binutils-gdb
```

The `.deb` file is also attached to the
[releases](https://gitlab.com/spseol/mit-no/stm8-binutils-gdb/-/releases) (from `v15.0.50-stm8.2`,
package version `15.0.50~20240111-2` with the fixes of `stm8-as` and `stm8-ld`). Install it with
`sudo apt install ./stm8-binutils-gdb_*_amd64.deb`. It needs `libpython3.13` (Debian 13 or similar).

### macOS (Apple Silicon)

A prebuilt package for macOS 12 or newer on Apple Silicon (arm64) is attached to the
[releases](https://gitlab.com/spseol/mit-no/stm8-binutils-gdb/-/releases) (use
`v15.0.50-stm8.2` or newer). It has GMP and MPFR linked statically and its own copy of
Python 3.14 (in `/usr/local/lib/stm8-python`), so `stm8-gdb` has Python and DAP support and
you do **not** need Homebrew, the Command Line Tools or an installed Python just to use it.
The bundled Python has no `ssl`, `curses` and `tkinter` modules; GDB and the
[STM8S toolchain](https://gitlab.com/spseol/mit-no/STM8S-toolchain) do not need them.
Intel Macs are not supported by the prebuilt package; build from source (see [Building](#building)).

The release has two files with the same content. Pick one:

| File | What it is | Choose it if |
|------|------------|--------------|
| `stm8-binutils-gdb-macos-arm64.pkg` | Standard macOS installer. Installs into `/usr/local` (`/usr/local/bin` is already in `PATH`). macOS remembers the files, so you can list or remove them later with `pkgutil`. | You want the usual double-click install. **Recommended for most people.** |
| `stm8-binutils-gdb-macos-arm64.tar.gz` | Plain archive with a `usr/local/` tree (`bin/`, `share/`, `lib/stm8-python/`, ...) and no installer. | You install from a script, or you want to look at the files before they land on the disk. |

The packages are **not signed or notarized**. For the installer this means that macOS
Gatekeeper blocks it at first.

Installer (`.pkg`):

1. Download the file. In Finder, right-click it and choose **Open**, then **Open** again
   (or allow it in *System Settings → Privacy & Security → Open Anyway*).
2. Follow the installer and enter your password.

Archive (`.tar.gz`):

```bash
sudo tar -xzf stm8-binutils-gdb-macos-arm64.tar.gz -C /
```

Without administrator rights you can unpack it into your home directory and put the `bin` directory
into `PATH` (add the second line to `~/.zprofile` to keep it):

```bash
mkdir -p ~/stm8 && tar -xzf stm8-binutils-gdb-macos-arm64.tar.gz -C ~/stm8
export PATH="$HOME/stm8/usr/local/bin:$PATH"
```

The tools find their files (including the bundled Python) relative to their own location, so the
directory can be anywhere. A program started from the Dock does not see the `PATH` of your shell, so
start VS Code / VSCodium from a terminal (`code .`) when the [STM8S toolchain](https://gitlab.com/spseol/mit-no/STM8S-toolchain)
extension has to find `stm8-gdb`.

Check the result in a new terminal window:

```bash
stm8-gdb --version
stm8-as --version
stm8-gdb -batch -ex 'python print(1)'     # must print 1
```

Do not use the older release `v15.0.50-stm8.1`: its `stm8-ld` writes wrong addresses and its
`stm8-gdb` has no Python.

Compare the SHA256 sum of the download with the one in the release notes
(`shasum -a 256 <file>`). The binaries were built by a community member on a Mac with
`contrib/build-macos.sh` (see [macOS package](#macos-package)), not by the maintainers; if you
prefer, build from source.

### From source

See [Building](#building) below.

## Building

### Requirements

You need a C and C++ compiler, GNU Make, bison, flex, and the libraries GMP and MPFR
(GDB needs them). GDB with Python scripting is needed by the STM8S toolchain (`make debug`,
VS Code extension), so install the Python 3 development files too. Texinfo is optional:
without it the documentation is not built.

- **Debian, Ubuntu:**
  ```bash
  sudo apt install build-essential bison flex texinfo libgmp-dev libmpfr-dev python3-dev
  ```
- **macOS:** Install the Command Line Tools (`xcode-select --install`). They have no GMP,
  MPFR or Texinfo, and their GNU Make and bison are old; the build works with them as is.
  The simplest way is `contrib/build-macos.sh` (see [macOS package](#macos-package) below): it
  builds GMP and MPFR itself and needs only [Python from python.org](https://www.python.org/downloads/macos/).
  To build by hand, install the libraries with [Homebrew](https://brew.sh):
  ```bash
  brew install gmp mpfr texinfo python
  ```

### Configure

Linux:

```bash
./configure \
  --prefix=/usr/local/ \
  --target=stm8-none-elf32 \
  --program-prefix=stm8-
```

macOS:

```bash
./configure \
  --prefix=/usr/local \
  --target=stm8-none-elf32 \
  --program-prefix=stm8- \
  --with-system-zlib \
  --with-gmp=$(brew --prefix gmp) \
  --with-mpfr=$(brew --prefix mpfr) \
  --with-python=$(brew --prefix python)/bin/python3 \
  --disable-nls --disable-werror \
  --disable-sim --disable-gprofng --disable-gdbserver
```

### Build and Install

Linux:

```bash
make -j$(nproc)
make install
```

macOS (`nproc` does not exist there):

```bash
make -j$(sysctl -n hw.ncpu)
make install
```

If you did not install Texinfo, add `MAKEINFO=true` to both `make` commands. The build
then skips the documentation (without it `make` stops with `doc/bfd.info: Error 127`).

### Configuration Options

- `--prefix`: Installation directory (use stow for easy management)
- `--target=stm8-none-elf32`: Target architecture
- `--program-prefix=stm8-`: Prefix for all tools (produces `stm8-gdb`, `stm8-objdump`, etc.)
- `--with-system-zlib`: Use the zlib of the system. The zlib bundled in the source tree
  does not compile with the current macOS SDK (`fdopen` is redefined).
- `--with-gmp`, `--with-mpfr`: Where Homebrew installed GMP and MPFR (macOS only).
- `--disable-sim`, `--disable-gprofng`, `--disable-gdbserver`: Skip parts that STM8 does not
  need and that only make the build longer or harder.
- `--disable-nls`, `--disable-werror`: No translations, and warnings of a newer compiler do
  not stop the build.
- `--with-python`: Build GDB with Python scripting. `./configure` turns it on by itself when it
  finds `python3-config`; on macOS the Command Line Tools have none, so the option is needed:
  `--with-python=/Library/Frameworks/Python.framework/Versions/3.14/bin/python3` (Python from
  python.org, tested) or `--with-python=$(brew --prefix python)/bin/python3` (Homebrew, not
  tested; it is a framework build of Python like the one from python.org). Check the result with
  `stm8-gdb -batch -ex 'python print(1)'`; it must print `1`. Python of the Command Line Tools
  (`Python3.framework` 3.9) works too, but its `sysconfig` points to files that do not exist, so
  `--with-python=$PWD/contrib/macos-clt-python-config.sh` is needed (it prints the right
  `--includes`, `--ldflags` and `--exec-prefix`; GDB's `configure` accepts such a script).
  `stm8-gdb` then needs the Command Line Tools at run time.
- `--disable-gdb`: Build only the binutils (`stm8-as`, `stm8-ld`, ...), then GMP and MPFR are
  not needed.

### macOS package

`contrib/build-macos.sh` builds everything from a clean state and makes the `.pkg` and
`.tar.gz` of the prebuilt package (Apple Silicon, macOS 12 or newer):

```bash
contrib/build-macos.sh ~/stm8-build-out     # results in ~/stm8-build-out/out/
```

It needs the Command Line Tools and Python 3.14 from python.org installed in
`/Library/Frameworks/Python.framework`. It downloads GMP and MPFR from ftp.gnu.org (checks the
SHA-256 sums) and links them statically, configures with the options above, installs into a
staging directory, and copies the Python runtime into `/usr/local/lib/stm8-python`, so
`stm8-gdb` works with Python without Homebrew or any installed Python. All binaries are signed ad hoc
(`codesign -s -`); the package is not signed or notarized.

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

