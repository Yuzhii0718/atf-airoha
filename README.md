# TF-A Airoha

Based on [ARM Trusted Firmware-A 2.15](https://github.com/ARM-software/arm-trusted-firmware/tree/v2.15) + Airoha ATF

> [!NOTE]
> Version Information:
>
> ARM Trusted Firmware-A 2.15: da738d5eae93af342fdc4995dd3c05acb4c9d757

## Quick Start

Prepare Environment:

```bash
sudo apt install -y lzma lzma-dev gcc-aarch64-linux-gnu build-essential unzip xz-utils
```

Toolchain:

```bash
wget -O dl/arm-gnu-toolchain-15.3.rel1-x86_64-arm-none-eabi.tar.xz https://gitlab.arm.com/api/v4/projects/tooling%2Fgnu-toolchains-for-arm/packages/generic/gnu-toolchain/15.3.rel1/arm-gnu-toolchain-15.3.rel1-x86_64-arm-none-eabi.tar.xz
```

MbedTLS:

```bash
wget -O dl/mbedtls-0bebf8b8c7f07abe3571ded48a11aa907a1ffb20.zip https://github.com/Mbed-TLS/mbedtls/archive/0bebf8b8c7f07abe3571ded48a11aa907a1ffb20.zip
```

## Build

```bash
SOC=<an7581|an7583|an7552|en7523> ./build.sh             # build all
SOC=<en7523>                      ./build.sh bl1         # build open-source BL1 (en7523 only)
SOC=<an7581|an7583|an7552|en7523> ./build.sh bl2         # build bl2
SOC=<an7581|an7583|an7552|en7523> ./build.sh bl31        # build bl31
SOC=<an7581|an7583|an7552|en7523> ./build.sh --help      # show usage
```

> [!NOTE]
> Build targets:
>
> - **an7581 / an7583** — BL2 (AArch32) + BL31 (AArch64), built from source.
> - **an7552** — BL2 (AArch32) + BL31 (AArch64), built from source.
> - **en7523** — BL2 (AArch32) + BL31 (AArch64, with `EFUSE_DISABLE`) + open-source BL1
>   (built from the reimplementation under `plat/ecnt/en7523/bl1/`, a flat AArch32
>   binary ≤2KB). No prebuilt BL1/BL31 blobs are used. The standalone `bl1` target
>   is only valid for en7523.

## Pipeline

### BL1 (open-source, en7523 only)
1. Built directly with the AArch32 toolchain (`arm-none-eabi-`, falls back to `arm-linux-gnueabihf-`)
2. Standalone flat binary linked at `0x0`, ≤2KB (asserted by `bl1.ld`)
3. Output: `en7523-bl1.bin`

### BL2 (3-stage and packaging)
1. **BL21** — Stage 1 loader (BL2 raw binary, uncompressed)
2. **BL22** — Stage 2 loader (BL2 LZMA-compressed)
3. **BL23** — Stage 3 loader (BL2 LZMA-compressed)
4. **spi_nand_flash_table** — Flash Table generation tool (compiled from source)
5. **Flash Table** — Generated and LZMA-compressed
6. **Packaging** — `airoha_pack_bl2.sh` concatenates BL21 + BL22.lzma + BL23.lzma + FlashTable.lzma into the final BL2 firmware

### BL31 (ARM64 runtime firmware)
1. aarch64 cross-compilation
2. LZMA-compressed output
