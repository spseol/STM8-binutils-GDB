#!/bin/sh
# "python program" for GDB's --with-python: GDB calls it as
#   <this> <srcdir>/python/python-config.py --includes|--ldflags|--exec-prefix
# The sysconfig data of Python3.framework in the Command Line Tools point to Xcode.app paths
# that do not exist, so the correct values are given here.
FW=/Library/Developer/CommandLineTools/Library/Frameworks
V=$FW/Python3.framework/Versions/3.9
for opt in "$@"; do
  case "$opt" in
    --includes)    echo "-I$V/include/python3.9" ;;
    --ldflags)     echo "-L$V/lib -lpython3.9 -Wl,-rpath,$FW" ;;
    --exec-prefix) echo "$V" ;;
  esac
done
