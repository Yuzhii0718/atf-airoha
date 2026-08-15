#!/bin/bash
#===============================================================================
# build.sh - Universal SOC (an7581 / an7583) BL2/BL31 firmware build script
#
# Usage: SOC=<an7581|an7583> ./build.sh [bl2|bl31|all]
#===============================================================================

set -e

#------------------------------------------------------------------------------
# SOC parameter check
#------------------------------------------------------------------------------
SOC="${SOC,,}" # Transform to lowercase

if [ -z "${SOC}" ]; then
    echo -e "\033[0;31m[ERROR]\033[0m not specified SOC environment variable."
    echo "Usage: SOC=<an7581|an7583> $0 [bl2|bl31|all]"
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
    *)
        echo -e "\033[0;31m[ERROR]\033[0m Not supported SOC: ${SOC}"
        exit 1
        ;;
esac

SOC_UPPER="${SOC^^}"

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
COMMON_BL2_FLAGS="\
    PLAT=${PLAT} \
    ARCH=aarch32 \
    ARM32TOOLCHAIN_BASE=${AARCH32_CROSS} \
    MBEDTLS_DIR=${MBEDTLS_DIR} \
    TOOLS_DIR=${LZMA_WRAPPER_DIR}/ \
    CONFIG_ECNT=1 \
    TCSUPPORT_OPENWRT=1 \
    TCSUPPORT_ATF_UNOPEN=0 \
    ${TCSUPPORT_FLAG} \
    TCSUPPORT_CPU_EN7523=1 \
    TCSUPPORT_CPU_ARMV8=1 \
    TCSUPPORT_UBOOT_64BIT=1 \
    TCSUPPORT_EMMC=1 \
    TCSUPPORT_UBOOT=1 \
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

    # --- AARCH64 Toolchain ---
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
    if [ ! -f "${SPI_NAND_FLASH_TABLE}" ]; then
        build_spi_nand_flash_table
    fi

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
build_bl2() {
    step "Build BL2 (3 stages: BL21, BL22, BL23) [${SOC_UPPER}]"

    cd "${ATF_DIR}"

    # --- BL21: 1st stage (Not compressed) ---
    info "[1/3] Build BL21..."
    make ${COMMON_BL2_FLAGS} clean
    make -j$(nproc) ${COMMON_BL2_FLAGS} IMAGE_BL21=1 bl2
    if [ ! -f "bl21.bin" ]; then
        error "bl21.bin not generated"
        exit 1
    fi
    info "BL21: $(stat -c%s bl21.bin) bytes"

    # --- BL22: 2nd stage (lzma) ---
    info "[2/3] Build BL22..."
    make ${COMMON_BL2_FLAGS} clean
    make -j$(nproc) ${COMMON_BL2_FLAGS} IMAGE_BL22=1 bl2
    if [ ! -f "bl22.lzma" ]; then
        error "bl22.lzma not generated"
        exit 1
    fi
    info "BL22: $(stat -c%s bl22.lzma) bytes (lzma)"

    # --- BL23: 3rd stage (lzma) ---
    info "[3/3] Build BL23..."
    make ${COMMON_BL2_FLAGS} clean
    make -j$(nproc) ${COMMON_BL2_FLAGS} IMAGE_BL23=1 bl2
    if [ ! -f "bl23.lzma" ]; then
        error "bl23.lzma not generated"
        exit 1
    fi
    info "BL23: $(stat -c%s bl23.lzma) bytes (lzma)"

    # Pack the final BL2 (bl21.bin + bl22.lzma + bl23.lzma + flash_table.lzma) to the SOC corresponding name
    pack_bl2
}

#------------------------------------------------------------------------------
# Build BL31 (aarch64)
#------------------------------------------------------------------------------
build_bl31() {
    step "Build BL31 (aarch64) [${SOC_UPPER}]"

    cd "${ATF_DIR}"
    make ${COMMON_BL31_FLAGS} clean
    make -j$(nproc) ${COMMON_BL31_FLAGS} bl31

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
    for f in "${SOC}-bl2.bin" "${SOC}-bl31.lzma" bl31.bin bl21.bin bl22.lzma bl23.lzma; do
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
            echo "Usage: SOC=<an7581|an7583> $0 [bl2|bl31|all]"
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
