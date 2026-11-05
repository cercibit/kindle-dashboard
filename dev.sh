#!/bin/sh
# Run project commands with the portable Lenovo build tools.
set -eu
cd "$(dirname "$0")"
PATH="$PWD/.tools/usr/bin:$PATH"
export PATH
exec "$@"
