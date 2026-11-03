#!/bin/sh
# Build stm8-binutils-gdb on macOS (Apple Silicon) and make a .pkg and a .tar.gz.
#
# Result: GDB with Python (and DAP), GMP and MPFR linked statically, the Python 3 runtime
# bundled in /usr/local/lib/stm8-python, no dependency on Homebrew.
#
# Requirements (build machine only):
#   - Apple Command Line Tools (xcode-select --install)
#   - Python 3.14 from https://www.python.org/downloads/macos/ (the "macOS 64-bit universal2
#     installer"), installed as /Library/Frameworks/Python.framework
#   - network access (GMP and MPFR sources are downloaded from ftp.gnu.org)
#
# Usage: contrib/build-macos.sh [work-dir]      (default work-dir: ~/stm8-build-out)
set -eu

SRC=$(cd "$(dirname "$0")/.." && pwd)
WORK=${1:-$HOME/stm8-build-out}
PYVER=${PYVER:-3.14}
PYFW=/Library/Frameworks/Python.framework/Versions/$PYVER
GMP_VER=6.3.0
GMP_SHA256=a3c2b80201b89e68616f4ad30bc66aee4927c3ce50e33929ca819d5c43538898
MPFR_VER=4.2.1
MPFR_SHA256=277807353a6726978996945af13e52829e3abd7a9a5b7fb2793894e18f1fcbb2
PKG_VERSION=${PKG_VERSION:-15.0.50}
JOBS=$(sysctl -n hw.ncpu)

# Oldest macOS the binaries run on.
MACOSX_DEPLOYMENT_TARGET=12.0
export MACOSX_DEPLOYMENT_TARGET

die() { echo "build-macos.sh: $*" >&2; exit 1; }

[ "$(uname -s)" = Darwin ] && [ "$(uname -m)" = arm64 ] || die "needs macOS on Apple Silicon"
[ -x "$PYFW/bin/python3" ] || die "Python $PYVER from python.org not found in $PYFW"

mkdir -p "$WORK/dl" "$WORK/logs"
DEPS=$WORK/deps
STAGE=$WORK/stage
LOG=$WORK/logs

fetch() { # url sha256 file
    [ -f "$WORK/dl/$3" ] || curl -fsSL -o "$WORK/dl/$3" "$1"
    [ "$(shasum -a 256 "$WORK/dl/$3" | cut -d' ' -f1)" = "$2" ] || die "wrong SHA-256 of $3"
}

# 1. GMP and MPFR, static only (so the binaries need no Homebrew libraries)
if [ ! -f "$DEPS/lib/libmpfr.a" ]; then
    fetch "https://ftp.gnu.org/gnu/gmp/gmp-$GMP_VER.tar.xz" "$GMP_SHA256" "gmp-$GMP_VER.tar.xz"
    fetch "https://ftp.gnu.org/gnu/mpfr/mpfr-$MPFR_VER.tar.xz" "$MPFR_SHA256" "mpfr-$MPFR_VER.tar.xz"
    rm -rf "$WORK/gmp-$GMP_VER" "$WORK/mpfr-$MPFR_VER"
    tar -xf "$WORK/dl/gmp-$GMP_VER.tar.xz" -C "$WORK"
    tar -xf "$WORK/dl/mpfr-$MPFR_VER.tar.xz" -C "$WORK"
    (cd "$WORK/gmp-$GMP_VER" &&
        ./configure --prefix="$DEPS" --disable-shared &&
        make -j"$JOBS" && make install) > "$LOG/gmp.log" 2>&1
    (cd "$WORK/mpfr-$MPFR_VER" &&
        ./configure --prefix="$DEPS" --with-gmp="$DEPS" --disable-shared &&
        make -j"$JOBS" && make install) > "$LOG/mpfr.log" 2>&1
fi

# 2. Configure and build (out of tree). MAKEINFO=true: Texinfo is not needed.
rm -rf "$WORK/build" "$STAGE"
mkdir -p "$WORK/build"
(cd "$WORK/build" &&
    "$SRC/configure" \
        --prefix=/usr/local \
        --target=stm8-none-elf32 \
        --program-prefix=stm8- \
        --with-system-zlib \
        --with-gmp="$DEPS" \
        --with-mpfr="$DEPS" \
        --with-python="$PYFW/bin/python3" \
        --disable-nls --disable-werror \
        --disable-sim --disable-gprofng --disable-gdbserver) > "$LOG/configure.log" 2>&1
(cd "$WORK/build" &&
    make -j"$JOBS" MAKEINFO=true > "$LOG/make.log" 2>&1 &&
    make install DESTDIR="$STAGE" MAKEINFO=true > "$LOG/install.log" 2>&1)

# 3. Bundle the Python runtime (arm64 only, without tests, tkinter, IDLE and extensions
#    that link to libraries outside of the framework: ssl, hashlib, curses, tkinter, zstd)
ROOT=$STAGE/usr/local
BUNDLE=$ROOT/lib/stm8-python/Python.framework/Versions/$PYVER
mkdir -p "$BUNDLE/lib"
lipo "$PYFW/Python" -thin arm64 -output "$BUNDLE/Python"
rsync -a \
    --exclude=test --exclude=tests --exclude=idlelib --exclude=tkinter --exclude=turtledemo \
    --exclude=ensurepip --exclude=site-packages --exclude=__pycache__ --exclude=lib2to3 \
    --exclude=config-"$PYVER"-darwin \
    "$PYFW/lib/python$PYVER" "$BUNDLE/lib/"
for so in "$BUNDLE/lib/python$PYVER"/lib-dynload/*.so; do
    if lipo -info "$so" | grep -q "Architectures in the fat"; then
        lipo "$so" -thin arm64 -output "$so.tmp" && mv "$so.tmp" "$so"
    fi
    if otool -L "$so" | grep -q "/Library/Frameworks/Python.framework"; then
        rm "$so"
    fi
done
NEW=@executable_path/../lib/stm8-python/Python.framework/Versions/$PYVER/Python
install_name_tool -id "$NEW" "$BUNDLE/Python"
install_name_tool -change "$PYFW/Python" "$NEW" "$ROOT/bin/stm8-gdb"
# Changing install names invalidates signatures; arm64 binaries must be signed (ad hoc is enough).
find "$BUNDLE" -type f \( -name '*.so' -o -name Python \) -exec codesign -s - --force {} \; 2>/dev/null
codesign -s - --force "$ROOT/bin/stm8-gdb"

# 4. Checks: Python works, no reference to Homebrew or to the build machine's Python
GDB=$ROOT/bin/stm8-gdb
for f in "$ROOT"/bin/*; do
    if otool -L "$f" | grep -E '/opt/homebrew|/usr/local/Cellar|/Library/Frameworks/Python.framework'; then
        die "$f links to a library outside of the package"
    fi
done
env -i "$GDB" -batch -ex 'python import gdb.dap; print("dap ok")' 2>&1 | grep -q 'dap ok' ||
    die "stm8-gdb: Python or gdb.dap does not work"
codesign -v "$GDB" "$BUNDLE/Python" || die "bad signature"

# 5. Packages (no AppleDouble ._* files, files owned by root)
xattr -cr "$STAGE" 2>/dev/null || true
find "$STAGE" -name '._*' -delete
OUT=$WORK/out
mkdir -p "$OUT"
export COPYFILE_DISABLE=1
(cd "$STAGE" && tar --no-xattrs --uid 0 --gid 0 --uname root --gname wheel \
    -czf "$OUT/stm8-binutils-gdb-macos-arm64.tar.gz" usr)
# pkgbuild stores every extended attribute (macOS puts com.apple.provenance on all files made
# by apps, and it cannot be removed) as an AppleDouble file ._name in the payload. So pkgbuild
# only makes the PackageInfo and the Bom; the payload is made again with cpio (no xattrs) and
# the package is put together by hand.
PKGTMP=$WORK/pkgtmp
rm -rf "$PKGTMP"
pkgbuild --root "$STAGE" --ownership recommended \
    --identifier cz.spseol.stm8-binutils-gdb --version "$PKG_VERSION" \
    --install-location / "$PKGTMP.pkg" > "$LOG/pkgbuild.log" 2>&1
pkgutil --expand "$PKGTMP.pkg" "$PKGTMP"
lsbom "$PKGTMP/Bom" | grep -v '/\._' > "$PKGTMP/bom-list.txt"
mkbom -i "$PKGTMP/bom-list.txt" "$PKGTMP/Bom"
(cd "$STAGE" && find . | cpio -o --format odc --owner 0:0 2>/dev/null | gzip -9 > "$PKGTMP/Payload")
sed -i '' "s|numberOfFiles=\"[0-9]*\" installKBytes=\"[0-9]*\"|numberOfFiles=\"$(wc -l < "$PKGTMP/bom-list.txt" | tr -d ' ')\" installKBytes=\"$(du -sk "$STAGE" | cut -f1)\"|" \
    "$PKGTMP/PackageInfo"
rm -f "$PKGTMP/bom-list.txt" "$OUT/stm8-binutils-gdb-macos-arm64.pkg"
pkgutil --flatten "$PKGTMP" "$OUT/stm8-binutils-gdb-macos-arm64.pkg"
rm -rf "$PKGTMP" "$PKGTMP.pkg"
[ "$(pkgutil --payload-files "$OUT/stm8-binutils-gdb-macos-arm64.pkg" | grep -c '\._')" = 0 ] ||
    die "the .pkg contains ._ files"
[ "$(tar tzf "$OUT/stm8-binutils-gdb-macos-arm64.tar.gz" | grep -c '\._')" = 0 ] ||
    die "the .tar.gz contains ._ files"
(cd "$OUT" && shasum -a 256 stm8-binutils-gdb-macos-arm64.pkg stm8-binutils-gdb-macos-arm64.tar.gz > SHA256SUMS)
echo "Done: $OUT"
cat "$OUT/SHA256SUMS"
