#!/usr/bin/env bash
# Build and install pilight from this repository on Debian 13 "trixie" /
# Raspberry Pi OS trixie (arm64 or armhf).
#
#   git clone -b mqtt5 https://github.com/benj1m4n/pilight.git
#   cd pilight
#   contrib/trixie/build.sh
#
# Steps:
#   1. install build dependencies from Debian
#   2. build and install wiringX (fixed commit + Raspberry Pi 4 sysfs patch)
#   3. build and install pilight from this checkout
#   4. keep an existing /etc/pilight/config.json, install the systemd unit
#      (foreground mode) and restart the service if it is enabled
#
# Environment:
#   SRC_DIR=...          where wiringX is cloned (default: ~/src)
#   SKIP_DEPS=1          do not run apt-get
#   PILIGHT_CMAKE_ARGS   extra arguments for cmake (pilight)

set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
SRC_DIR="${SRC_DIR:-$HOME/src}"
WX_PATCH="$HERE/wiringx-pi4-sysfs.patch"
WIRINGX_COMMIT="60d76e02d8dd1b7141fe0df87daf815c5ebfb564"   # wiringX master, 2026-08-10

SUDO=""
if [[ $(id -u) -ne 0 ]]; then
	SUDO="sudo"
fi

if [[ ! -f "$REPO/CMakeLists.txt" || ! -f "$REPO/daemon.c" ]]; then
	echo "ERROR: $REPO does not look like a pilight checkout." >&2
	exit 1
fi

echo "==> 1/4 build dependencies"
if [[ "${SKIP_DEPS:-0}" != "1" ]]; then
	$SUDO apt-get update
	$SUDO apt-get install -y build-essential cmake git \
		libmbedtls-dev liblua5.1-0-dev libluajit-5.1-dev libpcap-dev
fi

echo "==> 2/4 wiringX"
mkdir -p "$SRC_DIR"
if [[ ! -d "$SRC_DIR/wiringX/.git" ]]; then
	git clone https://github.com/wiringX/wiringX.git "$SRC_DIR/wiringX"
fi
cd "$SRC_DIR/wiringX"
git reset -q --hard
git clean -qfdx
git fetch -q origin "$WIRINGX_COMMIT" 2>/dev/null || git fetch -q origin
git checkout -q "$WIRINGX_COMMIT"
git apply "$WX_PATCH"
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build -j"$(nproc)"
$SUDO cmake --install build
$SUDO ldconfig

echo "==> 3/4 pilight"
cd "$REPO"
rm -rf build
# shellcheck disable=SC2086
cmake -S . -B build ${PILIGHT_CMAKE_ARGS:-}
cmake --build build -j"$(nproc)"

echo "==> 4/4 install"
# "make install" moves an existing config.json to config.json.bak and
# installs the default configuration; put the user's configuration back.
HAD_CONFIG=0
[[ -f /etc/pilight/config.json ]] && HAD_CONFIG=1
$SUDO cmake --install build
$SUDO ldconfig
if [[ $HAD_CONFIG -eq 1 && -f /etc/pilight/config.json.bak ]]; then
	$SUDO mv /etc/pilight/config.json.bak /etc/pilight/config.json
	echo "    kept existing /etc/pilight/config.json"
fi
# The HTTPS webserver certificate is not installed by "make install".
if [[ ! -f /etc/pilight/pilight.pem ]]; then
	$SUDO cp res/pilight.pem /etc/pilight/pilight.pem
fi

if [[ -d /run/systemd/system ]]; then
	$SUDO chmod 644 /etc/systemd/system/pilight.service /etc/systemd/system/pilight-wait-for-route.service 2>/dev/null || true
	$SUDO install -D -m 644 "$HERE/pilight-foreground.conf" /etc/systemd/system/pilight.service.d/override.conf
	$SUDO systemctl daemon-reload
	if systemctl is-enabled --quiet pilight 2>/dev/null; then
		$SUDO systemctl restart pilight
		echo "    restarted the pilight service"
	fi
fi

echo
echo "Done:"
/usr/local/sbin/pilight-daemon -V || true
echo
echo "Next steps:"
echo "  1. edit /etc/pilight/config.json while the service is stopped"
echo "     (gpio-platform, hardware 433gpio, devices)"
echo "  2. test in the foreground:  sudo pilight-daemon -F -D"
echo "  3. run as a service:        sudo systemctl enable --now pilight"
