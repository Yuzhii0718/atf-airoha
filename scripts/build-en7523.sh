#!/bin/bash

BASE_DIR="$(pwd)"

CHIP="en7523"
OUT_DIR="${BASE_DIR}/out/${CHIP}"

BUILD_ATF_DIR="${BASE_DIR}/arm-trusted-firmware-2.10.0"

BSP_CFLAGS="-fsigned-char \
	    -Wno-error=date-time \
	    -Wno-error=missing-include-dirs -Wno-error=redundant-decls \
	    -DTCSUPPORT_SPI_NAND_FLASH_ECC_DMA \
	    -DTCSUPPORT_BL2_OPTIMIZATION \
	    -DTCSUPPORT_CPU_ARMV8 \
	    -DTCSUPPORT_CPU_EN7521 \
	    -DTCSUPPORT_CPU_EN7523 \
	    -DTCSUPPORT_CPU_EN7580 \
	    -DTCSUPPORT_CPU_MT7520 \
	    -DTCSUPPORT_KERNEL_API \
	    -DTCSUPPORT_LITTLE_ENDIAN \
	"
LDFLAGS=""

ATF_CFLAGS_COMMON="${BSP_CFLAGS} -Wl,--no-warn-execstack \
	-Wno-error=missing-include-dirs -Wno-error=redundant-decls \
	-DTCSUPPORT_UBI_SUPPORT -DOVERRIDE_UBI_START_ADDR=0x100000"

ATF_CROSS_STAGING_DIR_AARCH32="${BASE_DIR}/toolchain-arm"
ATF_CROSS_PATH_AARCH32="${ATF_CROSS_STAGING_DIR_AARCH32}/bin"
ATF_CROSS_COMPILE_AARCH32="${ATF_CROSS_PATH_AARCH32}/arm-openwrt-linux-"
ATF_LDFLAGS_AARCH32="${LDFLAGS} --no-warn-execstack"
ATF_CFLAGS_AARCH32="${ATF_CFLAGS_COMMON} --param=min-pagesize=0"

export BSP_CFLAGS

export TCSUPPORT_BB_FIX_UNOPEN=1
export TCSUPPORT_BL2_OPTIMIZATION=1
export TCSUPPORT_CPU_EN7523=1
export TCSUPPORT_UBI_SUPPORT=1
export TCSUPPORT_UBOOT=1

aarch32_clean() {
	echo ">>> ATF: cleanup (aarch32) ..."
	make -C "${BUILD_ATF_DIR}" PLAT=en7523 ARCH=aarch32 distclean         \
		STAGING_DIR=${ATF_CROSS_STAGING_DIR_AARCH32}                  \
		CROSS_COMPILE_PATH=${ATF_CROSS_PATH_AARCH32}                  \
		CROSS_COMPILE_ATF=${ATF_CROSS_COMPILE_AARCH32}                \
		BSP_CFLAGS="${ATF_CFLAGS_AARCH32}"                            \
		LDFLAGS="${ATF_LDFLAGS_AARCH32}"
}

aarch32_build_bl21() {
	echo ">>> ATF: building BL21 (aarch32) ..."
	make -C "${BUILD_ATF_DIR}" PLAT=en7523 ARCH=aarch32 bl2 IMAGE_BL21=1  \
		TOOLS_DIR=${BASE_DIR}/../bin/                                 \
		STAGING_DIR=${ATF_CROSS_STAGING_DIR_AARCH32}                  \
		CROSS_COMPILE_PATH=${ATF_CROSS_PATH_AARCH32}                  \
		CROSS_COMPILE_ATF=${ATF_CROSS_COMPILE_AARCH32}                \
		BSP_CFLAGS="${ATF_CFLAGS_AARCH32}"                            \
		LDFLAGS="${ATF_LDFLAGS_AARCH32}"
}

aarch32_build_bl22() {
	echo ">>> ATF: building BL22 (aarch32) ..."
	make -C "${BUILD_ATF_DIR}" PLAT=en7523 ARCH=aarch32 bl2 IMAGE_BL22=1  \
		TOOLS_DIR=${BASE_DIR}/../bin/                                 \
		STAGING_DIR=${ATF_CROSS_STAGING_DIR_AARCH32}                  \
		CROSS_COMPILE_PATH=${ATF_CROSS_PATH_AARCH32}                  \
		CROSS_COMPILE_ATF=${ATF_CROSS_COMPILE_AARCH32}                \
		BSP_CFLAGS="${ATF_CFLAGS_AARCH32}"                            \
		LDFLAGS="${ATF_LDFLAGS_AARCH32}"
}

aarch32_build_bl23() {
	echo ">>> ATF: building BL23 (aarch32) ..."
	make -C "${BUILD_ATF_DIR}" PLAT=en7523 ARCH=aarch32 bl2 IMAGE_BL23=1  \
		TCSUPPORT_BL2_OPTIMIZATION=1                                  \
		TCSUPPORT_UBI_SUPPORT=1                                       \
		TOOLS_DIR=${BASE_DIR}/../bin/                                 \
		STAGING_DIR=${ATF_CROSS_STAGING_DIR_AARCH32}                  \
		CROSS_COMPILE_PATH=${ATF_CROSS_PATH_AARCH32}                  \
		CROSS_COMPILE_ATF=${ATF_CROSS_COMPILE_AARCH32}                \
		BSP_CFLAGS="${ATF_CFLAGS_AARCH32}"                            \
		LDFLAGS="${ATF_LDFLAGS_AARCH32}"
}

if [ ! -e "${BUILD_ATF_DIR}/plat/ecnt/blobs/en7523/bl1.bin" ]; then
    echo
    echo "WARNING: EN7523 SoC blobs are missed. Please find missed blobs"
    echo "  in your Airoha EN7523 SDK and put them to"
    echo
    echo "    ${BUILD_ATF_DIR}/plat/ecnt/blobs/en7523/"
    echo
    echo "  subdirectory. The following blobs are required:"
    echo "    /"
    echo "    +-- bl1.bin"
    echo "    +-- bl22"
    echo "    |     +-- dramc.o"
    echo "    |     +-- dramc_pi_basic_api.o"
    echo "    |     +-- dramc_pi_calibration_api.o"
    echo "    |     +-- dramc_pi_main.o"
    echo "    |     +-- efuse_load.o"
    echo "    |     +-- efuse.o"
    echo "    |     +-- hal_io.o"
    echo "    +-- bl23"
    echo "    |     +-- cortex_a53.o"
    echo "    |     +-- ecnt_npu_img.o"
    echo "    |     +-- efuse_load.o"
    echo "    |     +-- efuse.o"
    echo "    +-- bl31.bin"
    exit 1
fi

export PATH="${BASE_DIR}/bin:${PATH}"

(    aarch32_clean \
  && aarch32_build_bl21 \
  && echo ------------------------------------------------ \
  && aarch32_clean \
  && aarch32_build_bl22 \
  && echo ------------------------------------------------ \
  && aarch32_clean \
  && aarch32_build_bl23 \
  && echo ------------------------------------------------ \
) || exit 1

if [ ! -e ../bin/trx-airoha ]; then
    echo "ERROR: ../bin/trx-airoha utility is missed, Can't continue"
    exit 1
fi

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

gcc -DFLASH_TABLE_OPEN -DTCSUPPORT_BL2_OPTIMIZATION \
	-I"${BUILD_ATF_DIR}/plat/ecnt/en7523/include" \
	-o "${OUT_DIR}/flash-table-airoha" \
	"${BASE_DIR}/arm-trusted-firmware-2.10.0/plat/ecnt/common/drivers/flash/spi_nand_flash_table.c"
"${OUT_DIR}/flash-table-airoha"
../bin/lzma e flash_table.bin flash_table.lzma
mv flash_table.lzma "${OUT_DIR}"
rm flash_table.bin

../bin/trx-airoha -z \
	"${BUILD_ATF_DIR}/bl21.bin"  \
	"${BUILD_ATF_DIR}/bl22.lzma" \
	"${BUILD_ATF_DIR}/bl23.lzma" \
	"${OUT_DIR}/flash_table.lzma"     \
	bl2.tmp
../bin/trx-airoha -x bl2.tmp bl2_crc.bin
mv bl2_crc.bin "${OUT_DIR}/${CHIP}-bl2.bin"
rm bl2.tmp

cp ${BUILD_ATF_DIR}/plat/ecnt/blobs/en7523/bl1.bin "${OUT_DIR}/${CHIP}-bl1.bin"
../bin/lzma e ${BUILD_ATF_DIR}/plat/ecnt/blobs/en7523/bl31.bin "${OUT_DIR}/${CHIP}-bl31.lzma"
