#!/bin/bash
#===============================================================================
# download-tools.sh - fetch the host-side archives the in-tree build.sh needs
#
# build.sh (the in-tree entry point documented in README.md) reads two archives
# from dl/ and extracts them on first use:
#
#   dl/arm-gnu-toolchain-15.3.rel1-x86_64-arm-none-eabi.tar.xz
#       ARM GNU Toolchain 15.3.rel1, bare-metal AArch32 (arm-none-eabi-), used
#       for BL2 and BL1.  build.sh extracts it next to this repository.
#   dl/mbedtls-72718dd87e087215ce9155a826ee5a66cfbe9631.zip
#       mbedTLS sources consumed by BL31/BL32.  build.sh extracts it to
#       <repo>/mbedtls-3.4.1/.
#
# This is the *download* half only: nothing is extracted here, so the script is
# safe to run ahead of a build (build.sh picks the archives up on its next run)
# and its output directory can be cached between CI runs.
#
# This is not build-prepare.sh: that script belongs to the older out-of-tree
# 'build-${SOC}.sh' flow and downloads a different set of packages.  build.sh
# only ever consumes dl/.
#
# Usage: ./scripts/download-tools.sh [OPTIONS]
#
# Options:
#   -f, --force        Re-download even when an archive already exists.
#   -h, --help         Show this help.
#
# Environment:
#   DL_DIR=<path>        Destination directory (default: <repo>/dl).
#   TOOLCHAIN_URL=<url>  Override the ARM GNU Toolchain package URL.
#   MBEDTLS_URL=<url>    Override the mbedTLS package URL.
#
# Exit status: 0 when every archive is present and non-empty, 1 otherwise.
#
# Examples:
#   ./scripts/download-tools.sh
#   ./scripts/download-tools.sh --force
#   DL_DIR=/tmp/dl ./scripts/download-tools.sh
#===============================================================================

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

info()  { echo -e "${GREEN}[INFO]${NC}  $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*"; }
step()  { echo -e "\n${BLUE}=== $* ===${NC}"; }
die()   { error "$*"; exit 1; }

#------------------------------------------------------------------------------
# --help / -h: print usage extracted from the header comment block
#------------------------------------------------------------------------------
usage() {
	sed -n '2,/^[^#]/p' "$0" | grep -E '^#( |$)' | sed 's/^# \?//'
	exit 0
}

#------------------------------------------------------------------------------
# Command line
#------------------------------------------------------------------------------
FORCE=0
while [ $# -gt 0 ]; do
	case "$1" in
		--help|-h|help)
			usage
			;;
		--force|-f)
			FORCE=1
			;;
		*)
			error "Unknown option: $1"
			echo "Try '$0 --help' for more information."
			exit 1
			;;
	esac
	shift
done

#------------------------------------------------------------------------------
# Paths and package definitions
#
# The URLs and file names are the ones documented in README.md ("Toolchain" and
# "MbedTLS" under Quick Start); keep the two in sync.
#------------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ATF_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
DL_DIR="${DL_DIR:-${ATF_DIR}/dl}"

TOOLCHAIN_PKG="arm-gnu-toolchain-15.3.rel1-x86_64-arm-none-eabi.tar.xz"
TOOLCHAIN_URL="${TOOLCHAIN_URL:-https://gitlab.arm.com/api/v4/projects/tooling%2Fgnu-toolchains-for-arm/packages/generic/gnu-toolchain/15.3.rel1/${TOOLCHAIN_PKG}}"
TOOLCHAIN_MIN=100000000   # ~153 MiB; anything smaller is a truncated download

MBEDTLS_PKG="mbedtls-72718dd87e087215ce9155a826ee5a66cfbe9631.zip"
MBEDTLS_URL="${MBEDTLS_URL:-https://github.com/Mbed-TLS/mbedtls/archive/72718dd87e087215ce9155a826ee5a66cfbe9631.zip}"
MBEDTLS_MIN=1000000       # ~6 MiB

#------------------------------------------------------------------------------
# Downloader
#
# curl is preferred (it is what most CI images ship), wget is the fallback.  The
# payload is written to a .part file and only renamed once it looks complete, so
# an interrupted run never leaves a truncated archive that build.sh would try to
# extract.
#------------------------------------------------------------------------------
DOWNLOADER=""
detect_downloader() {
	if command -v curl >/dev/null 2>&1; then
		DOWNLOADER="curl"
	elif command -v wget >/dev/null 2>&1; then
		DOWNLOADER="wget"
	else
		die "neither curl nor wget is available; install one of them first."
	fi
	info "Downloader: ${DOWNLOADER}"
}

fetch() {
	local url="$1" dest="$2" part="${2}.part"

	rm -f "${part}"
	if [ "${DOWNLOADER}" = "curl" ]; then
		curl -fL --retry 3 --retry-delay 2 -o "${part}" "${url}"
	else
		wget -O "${part}" "${url}"
	fi
	mv -f "${part}" "${dest}"
}

# ensure_archive <package name> <url> <minimum size in bytes>
ensure_archive() {
	local pkg="$1" url="$2" min="$3"
	local dest="${DL_DIR}/${pkg}"

	if [ -s "${dest}" ] && [ "${FORCE}" != "1" ]; then
		local have
		have=$(stat -c%s "${dest}")
		if [ "${have}" -ge "${min}" ]; then
			info "${pkg} already present (${have} bytes), skipping."
			return 0
		fi
		warn "${pkg} is too small (${have} bytes < ${min}), re-downloading."
	fi

	info "Downloading ${pkg} ..."
	info "  from: ${url}"
	fetch "${url}" "${dest}"

	if [ ! -s "${dest}" ]; then
		die "download failed: ${dest} is empty."
	fi
	local size
	size=$(stat -c%s "${dest}")
	if [ "${size}" -lt "${min}" ]; then
		die "${pkg} looks truncated (${size} bytes < ${min}); delete ${dest} and retry."
	fi
	info "${pkg}: ${size} bytes"
}

#------------------------------------------------------------------------------
# Main
#------------------------------------------------------------------------------
main() {
	echo ""
	echo "==========================================================================="
	echo "  atf-airoha tool download"
	echo "  Source: ${ATF_DIR}"
	echo "  dl/:    ${DL_DIR}"
	echo "==========================================================================="

	detect_downloader

	mkdir -p "${DL_DIR}"
	[ -w "${DL_DIR}" ] || die "dl directory is not writable: ${DL_DIR}"

	step "Fetching build.sh dependencies"
	ensure_archive "${TOOLCHAIN_PKG}" "${TOOLCHAIN_URL}" "${TOOLCHAIN_MIN}"
	ensure_archive "${MBEDTLS_PKG}" "${MBEDTLS_URL}" "${MBEDTLS_MIN}"

	echo ""
	echo "==========================================================================="
	echo "  All archives are in ${DL_DIR}/"
	echo "  build.sh extracts them on the next run."
	echo "==========================================================================="
}

main "$@"
