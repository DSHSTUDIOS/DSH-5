#!/bin/sh
set -eu

cd "$(dirname "$0")"
cad_file=cad/DSH-5-RevD-Deck-Assembly.step

verify() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum -c asset-checksums.sha256
    else
        shasum -a 256 -c asset-checksums.sha256
    fi
}

if [ -e "$cad_file" ]; then
    verify
    exit
fi

tmp_file=$(mktemp "${cad_file}.XXXXXXXX")
trap 'rm -f "$tmp_file"' EXIT
xz -dc "${cad_file}.xz" >"$tmp_file"
expected=$(awk 'NR == 1 { print $1 }' asset-checksums.sha256)
if command -v sha256sum >/dev/null 2>&1; then
    actual=$(sha256sum "$tmp_file" | awk '{print $1}')
else
    actual=$(shasum -a 256 "$tmp_file" | awk '{print $1}')
fi
if [ "$actual" != "$expected" ]; then
    printf '%s\n' 'Assembly checksum failed.' >&2
    exit 1
fi
mv "$tmp_file" "$cad_file"
verify
