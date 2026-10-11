#!/bin/dash
#
# test driver to run $1 in the appropriate environment
#

# $1 = test executable to run
exe=$1

export PATH="$runtime_root:${PATH}"

if [ "$1" = "./mingw/cygload" ]
then
    windows_runtime_root=$(cygpath -m $runtime_root)
    $mingwtestdir/cygrun "$exe -v -cygwin $windows_runtime_root/cygwin1.dll"
else
    # Executables beside their private DLL need no extra testing namespace.
    case "$exe" in
        testinst/bin/*|./testinst/bin/*) set -- -notesting "$exe" ;;
        *) set -- "$exe" ;;
    esac
    cygdrop "$mingwtestdir/cygrun" "$@"
fi
