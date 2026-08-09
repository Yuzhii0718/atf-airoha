# TF-A Airoha

Based on [ARM Trusted Firmware-A 2.10](https://github.com/ARM-software/arm-trusted-firmware/tree/v2.10) + [Airoha ATF](https://github.com/Ansuel/atf-airoha/tree/80519beaea5298bbc660cd8f345e08aba9bd9547).

> [!NOTE]
> Version Information:
>
> ARM Trusted Firmware-A 2.10: b6c0948400594e3cc4dbb5a4ef04b815d2675808
>
> Airoha ATF: 80519beaea5298bbc660cd8f345e08aba9bd9547

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
wget -O dl/mbedtls-72718dd87e087215ce9155a826ee5a66cfbe9631.zip https://github.com/Mbed-TLS/mbedtls/archive/72718dd87e087215ce9155a826ee5a66cfbe9631.zip
```

## Build

```bash
SOC=<an7581|an7583> ./build.sh       # build all
SOC=<an7581|an7583> ./build.sh bl2   # build bl2
SOC=<an7581|an7583> ./build.sh bl31  # build bl31
```

## Pipeline

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
