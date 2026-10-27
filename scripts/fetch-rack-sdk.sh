#!/usr/bin/env bash
# Download and verify the VCV Rack 2 SDK for Linux x64 into the given directory (default: ../Rack-SDK).
#   scripts/fetch-rack-sdk.sh [dest]
set -euo pipefail

version=2.6.4
sha256=3eb7cdf86524a7296e445c86d1d13d1935f5b7e68ef76a369ef1d3a240b49068
dest=${1:-../Rack-SDK}

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
curl -fsSLo "$tmp/sdk.zip" "https://vcvrack.com/downloads/Rack-SDK-${version}-lin-x64.zip"
echo "${sha256}  $tmp/sdk.zip" | sha256sum -c -
unzip -q "$tmp/sdk.zip" -d "$tmp"
rm -rf "$dest"
mv "$tmp/Rack-SDK" "$dest"
echo "Rack SDK ${version} in ${dest}"
