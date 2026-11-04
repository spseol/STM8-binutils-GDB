#!/bin/sh
# Build stm8-binutils-gdb on Windows (x64) with MSYS2 and make a .zip and a setup .exe.
#
# Result: GDB with Python (and DAP), the Python 3 runtime and all DLLs bundled in the package,
# so it runs on a Windows without MSYS2 or Python.
#
# Requirements (build machine only):
#   - MSYS2 (https://www.msys2.org), started as the "MSYS2 UCRT64" shell
#   - packages (once):
#       pacman -S --needed base-devel git zip unzip bison flex \
#           mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-gmp mingw-w64-ucrt-x86_64-mpfr \
#           mingw-w64-ucrt-x86_64-python mingw-w64-ucrt-x86_64-expat mingw-w64-ucrt-x86_64-zlib \
#           mingw-w64-ucrt-x86_64-readline mingw-w64-ucrt-x86_64-ntldd
#   - for the setup .exe: Inno Setup 6 (https://jrsoftware.org/isinfo.php); without it only
#     the .zip is made. ISCC.exe is found in PATH or in the usual places; ISCC=<path> overrides.
#   - the source tree must have LF line endings (.gitattributes takes care of that)
#
# Usage: contrib/build-windows.sh [work-dir]      (default work-dir: ~/stm8-build-out)
#
# Linux and macOS builds do not use this script or anything from contrib/windows.
set -eu

SRC=$(cd "$(dirname "$0")/.." && pwd)
WORK=${1:-$HOME/stm8-build-out}
PKG_VERSION=${PKG_VERSION:-15.0.50}
JOBS=$(nproc)
NAME=stm8-binutils-gdb-windows-x64

die() { echo "build-windows.sh: $*" >&2; exit 1; }

[ "${MSYSTEM:-}" = UCRT64 ] || die "run it in the 'MSYS2 UCRT64' shell (MSYSTEM=UCRT64)"
command -v ntldd > /dev/null || die "ntldd not found: pacman -S mingw-w64-ucrt-x86_64-ntldd"
if [ "$(head -c 20 "$SRC/configure" | od -An -c | grep -c '\\r')" != 0 ]; then
    die "configure has CRLF line endings; clone with 'git -c core.autocrlf=false clone ...'"
fi
PYTHON=/ucrt64/bin/python3
[ -x "$PYTHON" ] || die "$PYTHON not found: pacman -S mingw-w64-ucrt-x86_64-python"
PYVER=$("$PYTHON" -c 'import sys; print("%d.%d" % sys.version_info[:2])')

mkdir -p "$WORK/logs"
BUILD=$WORK/build
STAGE=$WORK/stage
LOG=$WORK/logs
ROOT=$STAGE/$NAME

# 1. Configure and build (out of tree). MAKEINFO=true: Texinfo is not needed.
rm -rf "$BUILD" "$STAGE"
mkdir -p "$BUILD"
echo "configure ..."
#    GCC 15 and newer use C23 by default, which the C sources of this old tree do not compile in.
(cd "$BUILD" &&
    CFLAGS='-O2 -std=gnu17' CXXFLAGS='-O2' "$SRC/configure" \
        --prefix=/usr/local \
        --target=stm8-none-elf32 \
        --program-prefix=stm8- \
        --with-system-zlib \
        --with-system-readline \
        --with-python="$PYTHON" \
        --disable-nls --disable-werror \
        --disable-sim --disable-gprofng --disable-gdbserver) > "$LOG/configure.log" 2>&1 ||
    die "configure failed, see $LOG/configure.log"
echo "make ..."
(cd "$BUILD" &&
    make -j"$JOBS" MAKEINFO=true > "$LOG/make.log" 2>&1 &&
    make install DESTDIR="$STAGE.tmp" MAKEINFO=true > "$LOG/install.log" 2>&1) ||
    die "make failed, see $LOG/make.log and $LOG/install.log"
mkdir -p "$STAGE"
mv "$STAGE.tmp/usr/local" "$ROOT"
rm -rf "$STAGE.tmp"

# Not needed in the package: debug information, libtool archives, headers, info and man pages.
for f in "$ROOT"/bin/*.exe; do strip "$f"; done
rm -rf "$ROOT/include" "$ROOT/share/info" "$ROOT/share/man" "$ROOT/lib/bfd-plugins" \
    "$ROOT"/lib/*.a "$ROOT"/lib/*.la

# 2. Python runtime (without tests, tkinter, IDLE, pip and byte code) next to the binaries:
#    Python finds its standard library in <prefix>/lib/pythonX.Y relative to the executable.
mkdir -p "$ROOT/lib"
(cd /ucrt64/lib && tar -cf - \
    --exclude=test --exclude=tests --exclude=idlelib --exclude=tkinter --exclude=turtledemo \
    --exclude=ensurepip --exclude=site-packages --exclude=__pycache__ --exclude=lib2to3 \
    --exclude='config-*' --exclude='_tkinter*' "python$PYVER") | (cd "$ROOT/lib" && tar -xf -)

# 3. All DLLs from /ucrt64/bin that the executables and the Python extensions need. The
#    Windows system DLLs (api-ms-win-*, kernel32, ucrtbase, ...) are not copied.
copy_dlls() { # file...
    for f in "$@"; do
        ntldd -R "$f" 2> /dev/null | sed -n 's/^[[:space:]]*\([^[:space:]]*\.dll\) => \(.*ucrt64.bin.*\.dll\) .*/\2/Ip' |
            while read -r dll; do
                cp -n "$(cygpath -u "$dll")" "$ROOT/bin/"
            done
    done
}
copy_dlls "$ROOT"/bin/*.exe
find "$ROOT/lib/python$PYVER" \( -name '*.dll' -o -name '*.pyd' \) | while read -r ext; do
    copy_dlls "$ext"
done
# Python itself (libpythonX.Y.dll) is found through stm8-gdb.exe, its DLL dependencies above.

# 4. Licenses
mkdir -p "$ROOT/share/licenses"
cp "$SRC"/COPYING3 "$SRC"/COPYING "$SRC"/COPYING.LIB "$SRC"/COPYING3.LIB "$ROOT/share/licenses/"

# 5. Checks: the binaries work without MSYS2 (clean PATH) and Python with DAP is there
GDB=$ROOT/bin/stm8-gdb.exe
CLEAN_PATH=/c/Windows/System32:/c/Windows
env -i SYSTEMROOT="${SYSTEMROOT:-C:\\Windows}" PATH="$CLEAN_PATH" "$GDB" --version > /dev/null ||
    die "stm8-gdb does not start without MSYS2 (a DLL is missing?)"
env -i SYSTEMROOT="${SYSTEMROOT:-C:\\Windows}" PATH="$CLEAN_PATH" "$GDB" -batch \
    -ex 'python import gdb.dap; print("dap ok")' 2>&1 | grep -q 'dap ok' ||
    die "stm8-gdb: Python or gdb.dap does not work without MSYS2"
for tool in as ld objdump; do
    env -i SYSTEMROOT="${SYSTEMROOT:-C:\\Windows}" PATH="$CLEAN_PATH" "$ROOT/bin/stm8-$tool.exe" --version > /dev/null ||
        die "stm8-$tool does not start without MSYS2"
done

# 6. Packages
OUT=$WORK/out
mkdir -p "$OUT"
rm -f "$OUT/$NAME.zip" "$OUT/$NAME-setup.exe" "$OUT/SHA256SUMS"
(cd "$STAGE" && zip -qr -9 "$OUT/$NAME.zip" "$NAME")

ISCC=${ISCC:-}
if [ -z "$ISCC" ]; then
    for c in "$(command -v ISCC.exe || true)" \
        "$LOCALAPPDATA/Programs/Inno Setup 6/ISCC.exe" \
        "/c/Program Files (x86)/Inno Setup 6/ISCC.exe" \
        "/c/Program Files/Inno Setup 6/ISCC.exe"; do
        [ -n "$c" ] && [ -x "$c" ] && ISCC=$c && break
    done
fi
if [ -n "$ISCC" ]; then
    # MSYS2 would convert the /D... and /Q options to paths
    MSYS2_ARG_CONV_EXCL='*' "$ISCC" /Q "/DAppVersion=$PKG_VERSION" "/DSourceDir=$(cygpath -w "$ROOT")" \
        "/DOutputDir=$(cygpath -w "$OUT")" "$(cygpath -w "$SRC/contrib/windows/stm8-binutils-gdb.iss")" ||
        die "Inno Setup failed"
else
    echo "Inno Setup (ISCC.exe) not found: only the .zip is made" >&2
fi
(cd "$OUT" && sha256sum "$NAME.zip" "$NAME-setup.exe" 2> /dev/null > SHA256SUMS || true)
echo "Done: $OUT"
cat "$OUT/SHA256SUMS"
