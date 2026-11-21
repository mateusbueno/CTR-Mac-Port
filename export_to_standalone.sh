#!/bin/bash
# Copies save data (memcards/), config (ctr_native.cfg) and mod state from the
# project folder to the standalone app's data dir. Existing files are backed up.
set -euo pipefail
cd "$(dirname "$0")/build/CTR Native.app/Contents/Resources"
DEST="$HOME/Library/Application Support/CTR-Native"
mkdir -p "$DEST"
stamp=$(date +%Y%m%d-%H%M%S)
found=0
for item in memcards ctr_native.cfg mods_state.cfg; do
    [ -e "$item" ] && [ -s "$item" -o -d "$item" ] || continue
    if [ -e "$DEST/$item" ]; then mv "$DEST/$item" "$DEST/$item.bak-$stamp"; fi
    cp -R "$item" "$DEST/$item"; echo "Copied $item"; found=1
done
[ $found = 1 ] || echo "Nothing to export (no memcards/ or ctr_native.cfg in $(pwd))."
