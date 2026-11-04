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

### Windows (x64)

A prebuilt package for Windows 10 or newer (64-bit) is attached to the
[releases](https://gitlab.com/spseol/mit-no/stm8-binutils-gdb/-/releases). It contains `stm8-gdb`
with Python and DAP support, `stm8-as`, `stm8-ld`, `stm8-objdump` and the other binutils, the DLLs
they need and its own copy of Python 3.14, so you do **not** need MSYS2, Python or anything else
installed. The release has two files with the same content. Pick one:

| File | What it is | Choose it if |
|------|------------|--------------|
| `stm8-binutils-gdb-windows-x64-setup.exe` | Installer. Installs into `%ProgramFiles%\STM8 binutils-gdb` (administrator) or `%LocalAppData%\Programs\STM8 binutils-gdb` (just you), and adds `bin` to `PATH`. Uninstall it in *Settings → Apps*. | You want the usual install. **Recommended for most people.** |
| `stm8-binutils-gdb-windows-x64.zip` | Plain archive with the folder `stm8-binutils-gdb-windows-x64` (`bin\`, `lib\`, `share\`). | You do not have the rights to install, or you want a portable copy. |

The files are **not signed**, so Windows SmartScreen shows *Windows protected your PC* for the
installer: click *More info*, then *Run anyway*. For the `.zip`: unpack it where you want, and add
its `bin` directory to `PATH` (*Settings → System → About → Advanced system settings →
Environment Variables*, or `setx PATH "%PATH%;C:\tools\stm8-binutils-gdb-windows-x64\bin"`).

Check the result in a **new** terminal window (an open one has the old `PATH`):

```bat
stm8-gdb --version
stm8-as --version
stm8-gdb -batch -ex "python print(1)"
```

The last command must print `1`. Compare the SHA256 sum of the download with the one in the
release notes (`certutil -hashfile <file> SHA256`). The binaries were built with
`contrib/build-windows.sh` (see [Windows package](#windows-package)), not by the maintainers of
GNU binutils or GDB; if you prefer, build from source.

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

- **Windows:** The build needs a POSIX environment, so use [MSYS2](https://www.msys2.org) (the
  Windows build was done with the *UCRT64* environment). Install MSYS2, open the **MSYS2 UCRT64**
  shell from the Start menu (not the plain *MSYS2 MSYS* one) and install the packages once:
  ```bash
  pacman -Syu          # close the window when it asks, open it again and repeat until it says nothing to do
  pacman -S --needed base-devel git zip unzip bison flex \
      mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-gmp mingw-w64-ucrt-x86_64-mpfr \
      mingw-w64-ucrt-x86_64-python mingw-w64-ucrt-x86_64-expat mingw-w64-ucrt-x86_64-zlib \
      mingw-w64-ucrt-x86_64-readline mingw-w64-ucrt-x86_64-ntldd
  ```
  Use `contrib/build-windows.sh` (see [Windows package](#windows-package) below), which also makes
  the installer. MSYS2 with these packages takes about 3 GB and the build about 1 GB of disk space; the build
  takes about 30 minutes (8 cores).

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

### Windows package

`contrib/build-windows.sh` builds everything from a clean state and makes the `.zip` and the
setup `.exe` of the prebuilt package (Windows 10 or newer, x64). It is the only thing that
the Windows build adds; the Linux and macOS builds do not use it (nor `contrib/windows/`).

1. Install [MSYS2](https://www.msys2.org) (`winget install MSYS2.MSYS2`) and its packages, see
   [Requirements](#requirements).
2. Optional, for the setup `.exe`: install [Inno Setup 6](https://jrsoftware.org/isinfo.php)
   (`winget install JRSoftware.InnoSetup`). Without it only the `.zip` is made.
3. Get the sources **without line-ending conversion**. Git for Windows usually has
   `core.autocrlf=true`, which turns the files into CRLF and then `configure` stops with
   `syntax error near unexpected token`. The `.gitattributes` of this repository turns the conversion
   off, but when you clone, it is safest to say it too:
   ```bash
   git -c core.autocrlf=false clone https://gitlab.com/spseol/mit-no/stm8-binutils-gdb.git
   ```
4. Open the **MSYS2 UCRT64** shell and run (the work directory must not have spaces in its path):
   ```bash
   cd stm8-binutils-gdb
   contrib/build-windows.sh /c/stm8-build-out      # results in /c/stm8-build-out/out/
   ```
   Logs are in `…/logs/` (`configure.log`, `make.log`, `install.log`). Set `PKG_VERSION=15.0.50` to
   change the version in the installer; `ISCC=<path to ISCC.exe>` if Inno Setup is not found.

What the script does, which you need to know if you build by hand:

* configures with `CFLAGS='-O2 -std=gnu17'`: GCC 15 and newer compile C as C23, and the old C
  sources in this tree (`bfd`, `readline`) do not compile as C23;
* uses `--with-system-readline` (the readline in the tree fails with a new GCC) and
  `--with-python=/ucrt64/bin/python3` (the Python of MSYS2, built with MinGW like GDB; the
  python.org Python is built with MSVC and was not tried);
* installs into a staging directory and copies the Python standard library into `lib/python3.14`
  next to `bin` (Python finds it relative to `stm8-gdb.exe`) and every DLL that `ntldd` reports for
  the executables and Python extensions into `bin`;
* checks the result with a clean `PATH` (only `C:\Windows`): `stm8-gdb` must start and
  `import gdb.dap` must work, so no MSYS2 file is needed on the target machine;
* makes the `.zip` (`zip`) and the installer (`contrib/windows/stm8-binutils-gdb.iss`), which
  can install for one user or for all users and edits `PATH` accordingly.

The binaries are not signed. Upload `stm8-binutils-gdb-windows-x64-setup.exe`,
`stm8-binutils-gdb-windows-x64.zip` and `SHA256SUMS` to the release.

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

