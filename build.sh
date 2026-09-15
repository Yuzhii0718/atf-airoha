#!/bin/bash
#===============================================================================
# build.sh - Universal SOC (an7581 / an7583 / an7552 / en7523) BL2/BL31 build
#
# Usage: SOC=<an7581|an7583|an7552|en7523> [OPTEE=yes|no] ./build.sh [bl2|bl31|all]
#   OPTEE=yes enables OP-TEE (BL32) support, default: no
#
#   an7581 / an7583 : BL2 (aarch32) + BL31 built from source (aarch64).
#   an7552 : BL2 (aarch32) + BL31 built from source (aarch32).
#   en7523: BL2 (aarch32) + BL31 built from source (aarch64, EFUSE_DISABLE),
#           and BL1 built from the open-source reimplementation under
#           plat/ecnt/en7523/bl1/ (flat AArch32 binary, <=2KB).
#           No prebuilt BL1/BL31 blobs are used anymore.
#
# Feature switches (UBI/GPT FIP storage, BL31 FIP offset override, SPI-NAND
# ECC DMA reads, OP-TEE) mirror the per-SOC scripts build-an7581.sh /
# build-an7583.sh / build-an7552.sh / build-en7523.sh under scripts/.
#===============================================================================

set -e

#------------------------------------------------------------------------------
# SOC parameter check
#------------------------------------------------------------------------------
SOC="${SOC,,}" # Transform to lowercase

if [ -z "${SOC}" ]; then
    echo -e "\033[0;31m[ERROR]\033[0m not specified SOC environment variable."
    echo "Usage: SOC=<an7581|an7583|an7552|en7523> [OPTEE=yes|no] $0 [bl2|bl31|all]"
    echo "Example: SOC=an7583 $0 all"
    exit 1
fi

case "${SOC}" in
    an7581)
        TCSUPPORT_FLAG="TCSUPPORT_CPU_EN7581=1"
        ;;
    an7583)
        TCSUPPORT_FLAG="TCSUPPORT_CPU_AN7583=1"
        ;;
    an7552)
        TCSUPPORT_FLAG="TCSUPPORT_CPU_AN7552=1"
        ;;
    en7523)
        TCSUPPORT_FLAG=""   # platform default, TCSUPPORT_CPU_EN7523 is set in the common flags
        ;;
    *)
        echo -e "\033[0;31m[ERROR]\033[0m Not supported SOC: ${SOC}"
        exit 1
        ;;
esac

# an7581 / an7583 / an7552 build BL31 from source (aarch64) and use the eMMC /
# GPT based FIP storage.  en7523 builds BL31 from source as well (AArch64, the
# only BL31 flavour TF-A supports) but with EFUSE_DISABLE, because the EN7523
# eFuse driver is only shipped as closed prebuilt objects for aarch32 BL2.
# en7523 BL1 is built from the open-source reimplementation under
# plat/ecnt/en7523/bl1/; prebuilt BL1/BL31 blobs are no longer used.
case "${SOC}" in
    an7552)
        SOC_EMMC_FLAG=""
        SOC_GPT_FLAG=""
        SOC_UBOOT64_FLAG=""
        SOC_CPU_DEFS=""
        ;;
    en7523)
        SOC_EMMC_FLAG=""
        SOC_GPT_FLAG=""
        SOC_UBOOT64_FLAG=""
        SOC_CPU_DEFS=""
        ;;
    *)
        SOC_EMMC_FLAG="TCSUPPORT_EMMC=1"
        SOC_GPT_FLAG="TCSUPPORT_GPT_ATF_SUPPORT=1"
        SOC_UBOOT64_FLAG="TCSUPPORT_UBOOT_64BIT=1"
        SOC_CPU_DEFS="-DTCSUPPORT_CPU_ARMV8_64 -DTCSUPPORT_CPU_EN7581"
        ;;
esac

SOC_UPPER="${SOC^^}"

#------------------------------------------------------------------------------
# Optional Feature Switches
#
# Aligned with the dedicated per-SOC build scripts (scripts/build-an7581.sh,
# scripts/build-an7583.sh, scripts/build-an7552.sh).
# They enable features added by later platform commits (UBI / GPT based FIP
# storage, configurable BL31 FIP offset on eMMC, SPI-NAND ECC DMA reads,
# OP-TEE).
#
# The -D switches below are NOT translated into compiler defines by the ATF
# makefile by itself, so they are delivered through the standard BSP_CFLAGS
# hook (the top-level Makefile appends BSP_CFLAGS to TF_CFLAGS whenever
# TCSUPPORT_UBOOT is set).
#------------------------------------------------------------------------------
OPTEE="${OPTEE:-no}"    # OPTEE=yes -> build BL2/BL31 with OP-TEE (BL32) support

BSP_CFLAGS="\
    -fsigned-char \
    -Wno-error=date-time \
    -Wno-error=missing-include-dirs \
    -Wno-error=redundant-decls \
    -DTCSUPPORT_SPI_NAND_FLASH_ECC_DMA \
    -DTCSUPPORT_BL2_OPTIMIZATION \
    -DTCSUPPORT_CPU_ARMV8 \
    -DTCSUPPORT_CPU_EN7521 \
    -DTCSUPPORT_CPU_EN7523 \
    -DTCSUPPORT_CPU_EN7580 \
    -DTCSUPPORT_CPU_MT7520 \
    -DTCSUPPORT_KERNEL_API \
    -DTCSUPPORT_LITTLE_ENDIAN \
    -DTCSUPPORT_UBI_SUPPORT \
    -DOVERRIDE_UBI_START_ADDR=0x100000 \
    ${SOC_CPU_DEFS}"

if [ -n "${SOC_GPT_FLAG}" ]; then
    BSP_CFLAGS="${BSP_CFLAGS} -DTCSUPPORT_GPT_ATF_SUPPORT -DOVERRIDE_PLAT_ECNT_BL31_FIP_OFFSET=0x84000"
fi

OPTEE_BL23_OPT=""   # extra make vars for BL23 when OP-TEE is enabled
OPTEE_BL31_OPT=""   # extra make vars for BL31 when OP-TEE is enabled
if [ "${OPTEE}" = "yes" ]; then
    BSP_CFLAGS="${BSP_CFLAGS} -DTCSUPPORT_OPTEE"
    OPTEE_BL23_OPT="TCSUPPORT_OPTEE=1"
    OPTEE_BL31_OPT="TCSUPPORT_OPTEE=1 SPD=opteed"
fi
export BSP_CFLAGS

#------------------------------------------------------------------------------
# Path and Toolchain Configuration
#------------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ATF_DIR="${SCRIPT_DIR}"
OUTPUT_DIR="${ATF_DIR}/output/${SOC}"

# Toolchain
AARCH32_TOOLCHAIN_DIR="${AARCH32_TOOLCHAIN_DIR:-${SCRIPT_DIR}/../arm-gnu-toolchain-15.3.rel1-x86_64-arm-none-eabi}"
AARCH32_CROSS="${AARCH32_TOOLCHAIN_DIR}/bin/arm-none-eabi-"
AARCH64_CROSS="aarch64-linux-gnu-"

# Lib and Tool
MBEDTLS_DIR="${ATF_DIR}/mbedtls-3.4.1"
SPI_NAND_FLASH_TABLE="${ATF_DIR}/build/spi_nand_flash_table"
LZMA_WRAPPER_DIR="${ATF_DIR}/tools/lzma_wrapper"
SYSTEM_LZMA="/usr/bin/lzma"
PACK_SCRIPT="${ATF_DIR}/scripts/airoha_pack_bl2.sh"

# Platform
PLAT="en7523"

#------------------------------------------------------------------------------
# Universal Build Flags for BL2 and BL31
#------------------------------------------------------------------------------
# Note: the top-level Makefile resolves the aarch32 compiler to ARM32TOOLCHAIN_BASE
# only for an7581/an7583. an7552 must set CROSS_COMPILE_ATF explicitly, so it is
# provided for every SOC (same toolchain prefix as ARM32TOOLCHAIN_BASE).
COMMON_BL2_FLAGS="\
    PLAT=${PLAT} \
    ARCH=aarch32 \
    ARM32TOOLCHAIN_BASE=${AARCH32_CROSS} \
    CROSS_COMPILE_ATF=${AARCH32_CROSS} \
    MBEDTLS_DIR=${MBEDTLS_DIR} \
    TOOLS_DIR=${LZMA_WRAPPER_DIR}/ \
    CONFIG_ECNT=1 \
    TCSUPPORT_OPENWRT=1 \
    TCSUPPORT_ATF_UNOPEN=0 \
    ${TCSUPPORT_FLAG} \
    TCSUPPORT_CPU_EN7523=1 \
    TCSUPPORT_CPU_ARMV8=1 \
    ${SOC_UBOOT64_FLAG} \
    ${SOC_EMMC_FLAG} \
    TCSUPPORT_UBOOT=1 \
    TCSUPPORT_UBI_SUPPORT=1 \
    ${SOC_GPT_FLAG} \
    TCSUPPORT_BB_FIX_UNOPEN=1 \
    TCSUPPORT_BL2_OPTIMIZATION=1"

COMMON_BL31_FLAGS="\
    PLAT=${PLAT} \
    ARCH=aarch64 \
    CROSS_COMPILE=${AARCH64_CROSS} \
    CONFIG_ECNT=1 \
    TCSUPPORT_OPENWRT=1 \
    TCSUPPORT_ATF_UNOPEN=0 \
    ${TCSUPPORT_FLAG} \
    TCSUPPORT_CPU_EN7523=1 \
    TCSUPPORT_CPU_ARMV8=1 \
    ${SOC_UBOOT64_FLAG} \
    ${SOC_EMMC_FLAG} \
    TCSUPPORT_UBOOT=1 \
    TCSUPPORT_UBI_SUPPORT=1 \
    ${SOC_GPT_FLAG} \
    TCSUPPORT_BB_FIX_UNOPEN=1 \
    TCSUPPORT_BL2_OPTIMIZATION=1 \
    MBEDTLS_DIR=${MBEDTLS_DIR}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

info()    { echo -e "${GREEN}[INFO]${NC}  $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*"; }
step()    { echo -e "\n${BLUE}=== $* ===${NC}"; }

#------------------------------------------------------------------------------
# Environment Check
#------------------------------------------------------------------------------
check_environment() {
    step "Environment Check [SOC: ${SOC_UPPER}]"

    # --- ARM32 Toolchain ---
    if [ ! -f "${AARCH32_CROSS}gcc" ]; then
        local TL_TARBALL="${SCRIPT_DIR}/dl/arm-gnu-toolchain-15.3.rel1-x86_64-arm-none-eabi.tar.xz"
        if [ -f "${TL_TARBALL}" ]; then
            warn "ARM32 Toolchain not found, automatically extracting from dl/ to ${AARCH32_TOOLCHAIN_DIR} ..."
            mkdir -p "$(dirname "${AARCH32_TOOLCHAIN_DIR}")"
            tar -xf "${TL_TARBALL}" -C "$(dirname "${AARCH32_TOOLCHAIN_DIR}")"
        fi
    fi
    if [ ! -f "${AARCH32_CROSS}gcc" ]; then
        error "ARM32 Toolchain not found: ${AARCH32_CROSS}gcc"
        error "Please manually extract: tar -xf dl/arm-gnu-toolchain-15.3.rel1-x86_64-arm-none-eabi.tar.xz -C .."
        error "Or set the environment variable: export AARCH32_TOOLCHAIN_DIR=/path/to/toolchain"
        exit 1
    fi
    info "ARM32: $(${AARCH32_CROSS}gcc --version | head -1)"

    # --- AARCH64 Toolchain (BL31 is always built from source) ---
    if ! command -v aarch64-linux-gnu-gcc &>/dev/null; then
        error "AARCH64 Toolchain not found: aarch64-linux-gnu-gcc"
        error "Please install: sudo apt install -y gcc-aarch64-linux-gnu"
        exit 1
    fi
    info "AARCH64: $(aarch64-linux-gnu-gcc --version | head -1)"

    # --- mbedtls ---
    if [ ! -d "${MBEDTLS_DIR}" ]; then
        local MBEDTLS_ZIP="${SCRIPT_DIR}/dl/mbedtls-72718dd87e087215ce9155a826ee5a66cfbe9631.zip"
        if [ -f "${MBEDTLS_ZIP}" ]; then
            warn "mbedtls not found, automatically extracting from dl/..."
            unzip -qo "${MBEDTLS_ZIP}" -d /tmp/
            mv /tmp/mbedtls-72718dd87e087215ce9155a826ee5a66cfbe9631 "${MBEDTLS_DIR}"
        fi
    fi
    [ -d "${MBEDTLS_DIR}" ] || { error "mbedtls not found: ${MBEDTLS_DIR}"; exit 1; }
    info "mbedtls: ${MBEDTLS_DIR}"

    # --- lzma ---
    [ -f "${SYSTEM_LZMA}" ] || { error "lzma not found, please install: sudo apt install -y lzma"; exit 1; }
    info "lzma: ${SYSTEM_LZMA}"

    # --- lzma wrapper ---
    chmod +x "${LZMA_WRAPPER_DIR}/lzma" 2>/dev/null || true

    # --- spi_nand_flash_table ---
    # Always rebuild the host tool. It hard-codes the flash table into itself at
    # compile time, so a stale cached binary would embed an outdated table into
    # bl2.bin (e.g. a newly added NAND ID never taking effect). Rebuilding for
    # every target (bl1/bl2/bl31/all) keeps the embedded table in sync.
    build_spi_nand_flash_table

    # --- pack script ---
    chmod +x "${PACK_SCRIPT}" 2>/dev/null || true

    mkdir -p "${OUTPUT_DIR}"
    mkdir -p "${ATF_DIR}/build"
    info "Environment Check passed"
}

#------------------------------------------------------------------------------
# Compile spi_nand_flash_table Tool
#------------------------------------------------------------------------------
build_spi_nand_flash_table() {
    step "Compile spi_nand_flash_table"

    mkdir -p "$(dirname "${SPI_NAND_FLASH_TABLE}")"

    # Clean any cached binary first, then rebuild from the current C source
    rm -f "${SPI_NAND_FLASH_TABLE}"

    local SRC="${ATF_DIR}/plat/ecnt/common/drivers/flash/spi_nand_flash_table.c"
    local INC="${ATF_DIR}/plat/ecnt/en7523/include"

    gcc -O2 \
        -DFLASH_TABLE_OPEN \
        -DTCSUPPORT_BL2_OPTIMIZATION \
        -I"${INC}" \
        -o "${SPI_NAND_FLASH_TABLE}" \
        "${SRC}"

    if [ ! -f "${SPI_NAND_FLASH_TABLE}" ]; then
        error "spi_nand_flash_table Compile failed: ${SPI_NAND_FLASH_TABLE} not generated"
        exit 1
    fi
    info "spi_nand_flash_table Compile completed: ${SPI_NAND_FLASH_TABLE}"
}

#------------------------------------------------------------------------------
# Pack BL2 Final (bl21.bin + bl22.lzma + bl23.lzma + flash_table.lzma)
#------------------------------------------------------------------------------
pack_bl2() {
    step "Generate Flash Table and pack BL2 [${SOC_UPPER}]"

    local PACK_TMP="${ATF_DIR}/build/pack_bl2_tmp"
    rm -rf "${PACK_TMP}"
    mkdir -p "${PACK_TMP}"

    # Copy BL2 stages to temporary directory
    cp "${ATF_DIR}/bl21.bin"  "${PACK_TMP}/"
    cp "${ATF_DIR}/bl22.lzma" "${PACK_TMP}/"
    cp "${ATF_DIR}/bl23.lzma" "${PACK_TMP}/"

    # Generate SPI NAND Flash Table (Output: flash_table.bin)
    info "Generate Flash Table..."
    "${SPI_NAND_FLASH_TABLE}" 2 \
        -partlen=4194304 \
        -tablesize=131072 \
        -offset=131072 \
        -blocksize=131072 \
        -sectorsize=2048 \
        -page_size=2048 \
        -oobsize=64
    if [ ! -f "flash_table.bin" ]; then
        error "Flash Table generate failed: flash_table.bin not found"
        exit 1
    fi

    # lzma compress Flash Table
    ${SYSTEM_LZMA} -z -c "flash_table.bin" > "${PACK_TMP}/flash_table.lzma"

    # pack the final BL2 (bl21.bin + bl22.lzma + bl23.lzma + flash_table.lzma) to the SOC corresponding name
    info "Pack the final BL2 (${SOC}-bl2.bin)..."
    bash "${PACK_SCRIPT}" \
        "${PACK_TMP}/bl21.bin" \
        "${PACK_TMP}/bl22.lzma" \
        "${PACK_TMP}/bl23.lzma" \
        "${PACK_TMP}/flash_table.lzma" \
        "${OUTPUT_DIR}/${SOC}-bl2.bin"

    # Keep the individual files for reference
    cp "${PACK_TMP}/bl21.bin"  "${OUTPUT_DIR}/"
    cp "${PACK_TMP}/bl22.lzma" "${OUTPUT_DIR}/"
    cp "${PACK_TMP}/bl23.lzma" "${OUTPUT_DIR}/"

    rm -rf "${PACK_TMP}"
    info "BL2 pack completed"
}

#------------------------------------------------------------------------------
# Build BL2 3 stages (BL21 -> BL22 -> BL23)
#------------------------------------------------------------------------------
# The BL2 stages and BL31 share a single build directory and only differ by the
# IMAGE_BL2x define / ARCH.  `make clean` can silently fail (e.g. a bulk-delete
# guard refuses to remove the object tree), in which case the next stage is
# linked against the previous stage's objects (or against libs built for the
# other architecture) and produces a truncated / unusable image.  Drop the
# object tree up-front so every stage starts from scratch.
# $(1) = output file of the stage about to be built (removed so a failed
#        build can never leave the previous stage's artifact behind)
# $(2) = make flags of the stage (used for `make clean`)
clean_atf_tree() {
    local out="$1"
    local flags="$2"
    local tree="${ATF_DIR}/build/${PLAT}/release"
    local attempt=0

    rm -f  "${ATF_DIR}/${out}" 2>/dev/null || true
    make ${flags} clean >/dev/null 2>&1 || true

    # `make clean` can fail silently (bulk delete refused) and it re-creates the
    # build directory, so drop the object tree afterwards and verify.  The bulk
    # delete is refused intermittently in some sandboxes, hence the retries, and
    # the tree is renamed aside as a fallback (it only has to disappear from
    # this path, the leftover can be cleaned up later).
    while [ -d "${tree}" ] && [ ${attempt} -lt 5 ]; do
        rm -rf "${tree}" 2>/dev/null || true
        if [ -d "${tree}" ]; then
            mv "${tree}" "${tree}.stale.$$" 2>/dev/null || true
        fi
        attempt=$((attempt + 1))
        if [ -d "${tree}" ]; then
            sleep 1
        fi
    done
    rm -rf "${ATF_DIR}/build/${PLAT}/debug" 2>/dev/null || true

    if [ -d "${tree}" ]; then
        error "Cannot remove the object tree: ${tree}"
        error "The next stage would be linked against the previous stage's objects."
        exit 1
    fi
}

# $(1) = output file of the BL2 stage about to be built
clean_bl2_tree() {
    clean_atf_tree "$1" "${COMMON_BL2_FLAGS}"
}

build_bl2() {
    step "Build BL2 (3 stages: BL21, BL22, BL23) [${SOC_UPPER}]"

    cd "${ATF_DIR}"

    # --- BL21: 1st stage (Not compressed) ---
    info "[1/3] Build BL21..."
    clean_bl2_tree bl21.bin
    make -j$(nproc) ${COMMON_BL2_FLAGS} IMAGE_BL21=1 bl2
    if [ ! -f "bl21.bin" ]; then
        error "bl21.bin not generated"
        exit 1
    fi
    info "BL21: $(stat -c%s bl21.bin) bytes"

    # --- BL22: 2nd stage (lzma) ---
    info "[2/3] Build BL22..."
    clean_bl2_tree bl22.lzma
    make -j$(nproc) ${COMMON_BL2_FLAGS} IMAGE_BL22=1 bl2
    if [ ! -f "bl22.lzma" ]; then
        error "bl22.lzma not generated"
        exit 1
    fi
    # BL22 carries the DRAM calibration code and is ~20 KB compressed.
    if [ "$(stat -c%s bl22.lzma)" -lt 8192 ]; then
        error "bl22.lzma looks truncated ($(stat -c%s bl22.lzma) bytes) - stale object tree?"
        exit 1
    fi
    info "BL22: $(stat -c%s bl22.lzma) bytes (lzma)"

    # --- BL23: 3rd stage (lzma) ---
    info "[3/3] Build BL23..."
    clean_bl2_tree bl23.lzma
    make -j$(nproc) ${COMMON_BL2_FLAGS} ${OPTEE_BL23_OPT} IMAGE_BL23=1 bl2
    if [ ! -f "bl23.lzma" ]; then
        error "bl23.lzma not generated"
        exit 1
    fi
    # BL23 is the full featured BL2 and is ~40 KB compressed.
    if [ "$(stat -c%s bl23.lzma)" -lt 20480 ]; then
        error "bl23.lzma looks truncated ($(stat -c%s bl23.lzma) bytes) - stale object tree?"
        exit 1
    fi
    info "BL23: $(stat -c%s bl23.lzma) bytes (lzma)"

    # Pack the final BL2 (bl21.bin + bl22.lzma + bl23.lzma + flash_table.lzma) to the SOC corresponding name
    pack_bl2
}

#------------------------------------------------------------------------------
# Build the open-source BL1
#
# A clean-room reimplementation of the vendor bl1.bin, kept under
# plat/ecnt/en7523/bl1/. It is a standalone flat binary (AArch32/ARMv7-A,
# linked at 0x0, <=2KB) so it is built directly with the aarch32 toolchain
# rather than through the ATF top-level Makefile. The prebuilt vendor blob is
# no longer used.
#------------------------------------------------------------------------------
build_bl1() {
    step "Build BL1 (open-source) [${SOC_UPPER}]"

    local BL1_DIR="${ATF_DIR}/plat/ecnt/en7523/bl1"

    if [ ! -d "${BL1_DIR}" ]; then
        error "open-source BL1 source not found: ${BL1_DIR}"
        exit 1
    fi

    # The dedicated build.sh aarch32 toolchain is arm-none-eabi- (bundled);
    # fall back to the distro arm-linux-gnueabihf- when it is not present.
    local BL1_CROSS="${AARCH32_CROSS}"
    if [ ! -x "${BL1_CROSS}gcc" ]; then
        BL1_CROSS="arm-linux-gnueabihf-"
    fi
    info "BL1 cross toolchain: ${BL1_CROSS}"

    make -C "${BL1_DIR}" CROSS_COMPILE="${BL1_CROSS}" clean >/dev/null 2>&1 || true
    if ! make -C "${BL1_DIR}" CROSS_COMPILE="${BL1_CROSS}"; then
        error "open-source BL1 build failed"
        exit 1
    fi

    mkdir -p "${OUTPUT_DIR}"
    cp "${BL1_DIR}/bl1.bin" "${OUTPUT_DIR}/${SOC}-bl1.bin"
    info "${SOC}-bl1.bin (source): $(stat -c%s ${OUTPUT_DIR}/${SOC}-bl1.bin) bytes"
}

#------------------------------------------------------------------------------
# Build BL31 from source (aarch64)
#------------------------------------------------------------------------------
build_bl31() {
    step "Build BL31 [${SOC_UPPER}]"

    cd "${ATF_DIR}"

    # EN7523: the eFuse driver is closed source and only shipped as aarch32
    # objects, so BL31 is built with the in-tree EFUSE_DISABLE configuration:
    # efuse_init() is not called and the eFuse SMC handler answers
    # "not supported" (see ecnt_plat_common.c) instead of failing to link.
    local BL31_MAKE_VARS=""
    local BL31_MAKE_ENV=()
    if [ "${SOC}" = "en7523" ]; then
        BL31_MAKE_VARS="EFUSE_DISABLE=1"
        BL31_MAKE_ENV=(env "BSP_CFLAGS=${BSP_CFLAGS} -DEFUSE_DISABLE")
    fi

    # BL2 and BL31 share build/${PLAT}/release: always start from an empty tree,
    # otherwise the BL31 link can pick up the aarch32 libc/libmbedtls built for BL2.
    clean_atf_tree "build/${PLAT}/release/bl31.bin" "${COMMON_BL31_FLAGS}"

    "${BL31_MAKE_ENV[@]}" make -j$(nproc) ${COMMON_BL31_FLAGS} \
        ${BL31_MAKE_VARS} ${OPTEE_BL31_OPT} bl31

    local BL31_BIN="build/${PLAT}/release/bl31.bin"
    if [ ! -f "${BL31_BIN}" ]; then
        error "BL31 build failed: ${BL31_BIN} not generated"
        exit 1
    fi

    # keep the original bl31.bin
    cp "${BL31_BIN}" "${OUTPUT_DIR}/bl31.bin"
    info "BL31 raw: $(stat -c%s ${OUTPUT_DIR}/bl31.bin) bytes"

    # lzma compression
    ${SYSTEM_LZMA} -z -c "${BL31_BIN}" > "${OUTPUT_DIR}/${SOC}-bl31.lzma"
    info "BL31 lzma: $(stat -c%s ${OUTPUT_DIR}/${SOC}-bl31.lzma) bytes"

    # en7523 also ships the open-source BL1 and, for the pre-existing flashing
    # flow, an unsuffixed copy of the freshly built bl31.lzma.
    if [ "${SOC}" = "en7523" ]; then
        build_bl1
        cp "${OUTPUT_DIR}/${SOC}-bl31.lzma" "${OUTPUT_DIR}/bl31.lzma"
    fi
}

#------------------------------------------------------------------------------
# print summary
#------------------------------------------------------------------------------
print_summary() {
    echo ""
    echo "==========================================================================="
    echo -e "  ${GREEN}${SOC_UPPER} firmware build completed!${NC}"
    echo "==========================================================================="
    echo ""
    echo "  Output directory: ${OUTPUT_DIR}/"
    echo ""

    local f size
    for f in "${SOC}-bl2.bin" "${SOC}-bl1.bin" "${SOC}-bl31.lzma" bl31.lzma bl31.bin bl21.bin bl22.lzma bl23.lzma; do
        if [ -f "${OUTPUT_DIR}/${f}" ]; then
            size=$(stat -c%s "${OUTPUT_DIR}/${f}")
            printf "    %-30s  %10s bytes\n" "$f" "$size"
        fi
    done
    echo ""
    echo "==========================================================================="
}

#------------------------------------------------------------------------------
# Main
#------------------------------------------------------------------------------
main() {
    local TARGET="${1:-all}"

    echo ""
    echo "==========================================================================="
    echo "  ${SOC_UPPER} BL2/BL31 firmware build script"
    echo "  Source: ${ATF_DIR}"
    echo "  (Full ATF 2.10 + atf-airoha ECNT platform code)"
    echo "  Output: ${OUTPUT_DIR}"
    echo "  OPTEE: ${OPTEE}"
    echo "==========================================================================="

    check_environment

    case "${TARGET}" in
        bl2)
            build_bl2
            ;;
        bl31)
            build_bl31
            ;;
        all|"")
            build_bl2
            build_bl31
            ;;
        *)
            echo "Usage: SOC=<an7581|an7583|an7552|en7523> [OPTEE=yes|no] $0 [bl2|bl31|all]"
            echo "  OPTEE=yes - build with OP-TEE (BL32) support"
            echo "  bl2  - Only build BL2 (including BL21/BL22/BL23 + packaging)"
            echo "  bl31 - Only build BL31"
            echo "  all  - Build everything (default)"
            exit 1
            ;;
    esac

    make ${COMMON_BL2_FLAGS} clean 2>/dev/null || true

    print_summary
}

main "$@"
