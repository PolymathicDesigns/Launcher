#!/bin/bash

INSTDIR="${XDG_DATA_HOME-$HOME/.local/share}/multimc"

if [ `getconf LONG_BIT` = "64" ]
then
    PACKAGE="mmc-stable-lin64.tar.gz"
else
    PACKAGE="mmc-stable-lin32.tar.gz"
fi

deploy() (
    set -o pipefail
    mkdir -p "$INSTDIR" || exit 1
    rm -f "$INSTDIR/.bootstrap-complete" || exit 1
    staging=$(mktemp -d "$INSTDIR/.bootstrap.XXXXXX") || exit 1
    trap 'rm -rf -- "$staging"' EXIT

    if ! wget -O "$staging/package.tar.gz" --progress=dot:force "https://files.multimc.org/downloads/${PACKAGE}" 2>&1 |
        sed -u 's/.* \([0-9]\+%\)\ \+\([0-9.]\+.\) \(.*\)/\1\n# Downloading at \2\/s, ETA \3/' |
        zenity --progress --auto-close --auto-kill --title="Downloading MultiMC..."
    then
        echo "MultiMC download failed or was cancelled. Start MultiMC again to retry." >&2
        exit 1
    fi
    mkdir "$staging/extracted" || exit 1
    if ! tar -xzf "$staging/package.tar.gz" --strip-components=1 -C "$staging/extracted"; then
        echo "MultiMC download could not be extracted. Start MultiMC again to retry." >&2
        exit 1
    fi
    if [[ ! -f "$staging/extracted/MultiMC" || ! -x "$staging/extracted/bin/MultiMC" ]]; then
        echo "MultiMC download is missing launcher files. Start MultiMC again to retry." >&2
        exit 1
    fi
    chmod +x "$staging/extracted/MultiMC" || exit 1
    cp -a "$staging/extracted/." "$INSTDIR/" || exit 1
    touch "$INSTDIR/.bootstrap-complete" || exit 1
)

runmmc() {
    cd "$INSTDIR" || exit 1
    exec ./MultiMC "$@"
}

if [[ ! -f "$INSTDIR/.bootstrap-complete" || ! -x "$INSTDIR/MultiMC" || ! -x "$INSTDIR/bin/MultiMC" ]]; then
    deploy || exit 1
fi
runmmc "$@"
