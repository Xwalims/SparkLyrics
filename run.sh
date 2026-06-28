#!/usr/bin/env bash
QT6=/nix/store/wzvm2pvf98a71v0scgfhmi01lqcxahfr-qtbase-6.11.0
DIR="$(dirname "$0")"

exec env LD_LIBRARY_PATH="$QT6/lib${LD_LIBRARY_PATH:+:}$LD_LIBRARY_PATH" \
  "$DIR/build/sparklyrics" "$@"
