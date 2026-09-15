# EN7523 vendor BL1 — reverse-engineering map & open-source reimplementation

Target blob: `plat/ecnt/blobs/en7523/bl1.bin`
- 2016 bytes (`0x7E0`), AArch32 / ARMv7-A, linked at base `0x0`
- md5 `228c9da65f253cf8326e19c41de245e4` (byte-identical to the SDK reference)
- loaded by the on-chip BootROM into a **≤2 KB window**, entry at offset `0`

Open-source replacement: `plat/ecnt/en7523/bl1/`
- `bl1_entry.S`  – reset entry, CP15 setup, MIDR dispatch, exception vectors
- `bl1_main.c`   – MCUCFG config, NPU-SRAM self-test/clear, SPI flash loader
- `bl1.ld`       – flat, base `0x0`, vectors at `0x7C0`, 2 KB size assertion
- `Makefile`     – standalone aarch32 flat-binary build (`bl1.bin`)
- build via `BL1_MODE=source SOC=en7523 ./build.sh all`

## 1. Boot chain

```
BootROM (mask ROM)
  -> BL1  (this blob, AArch32, 2 KB)   : CPU/cache setup, SRAM clear, flash load
  -> BL2  (aarch32, source)            : loaded into L2 SRAM @0x08000000
  -> BL31 (aarch64, source) -> BL33/U-Boot -> Linux
```

**Important:** the blob contains **no DDR/DRAMC access at all** (no `0x1FE00000`
literal). DDR bring-up is *not* done in BL1 — BL1 only clears the on-chip
NPU SRAM and loads the next stage into the L2 SRAM. (An earlier note claiming
BL1 touches the DRAMC was wrong.)

## 2. Function map (blob offset -> role -> open-source symbol)

| blob | role | open-source |
|------|------|-------------|
| `0x000` | 16× `nop` filler, real entry begins at `0x040` | `_bl1_start` (entry at `0x0`) |
| `0x040` | reset: SCTLR/cps/VBAR/…/SRAM init/relocate | `_bl1_start` |
| `0x140` | `wfi; b .` fatal hang | `bl1_panic` |
| `0x148` | `if (part<=3) impl_reg(c15,opc1=0) \|= BIT24` | `bl1_impl_reg_set_bit24` |
| `0x174` | `if (part>=3) impl_reg(c15,opc1=0)` RMW | `bl1_impl_reg_touch` |
| `0x19C` | Cortex-A53 core init | `bl1_a53_init` |
| `0x1CC` | MCUCFG (AXI/L2C) + NPU-SRAM self-test & clear | `bl1_plat_init` |
| `0x2AC` | SCU hand-off write + relocate `0x0 -> 0x1F000000` | `bl1_relocate` |
| `0x2F0` | relocated continuation: boot-media dispatch | `_bl1_resume` |
| `0x374` | SPI flash read state machine | `spi_flash_read` |
| `0x584` | SPI cmd (`op`,`mode`) | `spi_cmd` |
| `0x5E8` | SPI read byte | `spi_rx` |
| `0x620` | SPI write byte | `spi_tx` |
| `0x648` | early init: MIDR dispatch | `bl1_cpu_early_init` |
| `0x668` | MIDR table match (mask `0xFF00FFF0`) | `bl1_midr_match` |
| `0x6AC` | MIDR -> part number `((MIDR>>16)&0xFF) \| (MIDR&0xF)` | `bl1_midr_part` |
| `0x6BC` / `0x6CC` | `(a<=b)` / `(a>=b)` predicates | `bl1_cmp_le` / `bl1_cmp_ge` |
| `0x6DC` | set stack `sp = 0x1E824000` | (open BL1 sets SP earlier, see §6) |
| `0x6EC` | exception handler (hang) | `bl1_exc_handler` |
| `0x6FC` / `0x760` | `memset` / `memcpy` (called with len 0 in blob) | not needed (C) |
| `0x794`/`0x798`/`0x79C` | `bx lr` stubs / `b .` hang | folded into `bl1_panic` |
| `0x7A0` | MIDR table: `(0x410FD030, 0x19C)` `(0x410FD0F0, 0)` | `_bl1_cpu_table` |
| `0x7C0` | exception vectors (reset -> `b 0`, others -> `b 0x6EC`) | `_bl1_vectors` |

## 3. MMIO map

| address | symbol | use |
|---------|--------|-----|
| `0x1EFBE02C` | `MCUCFG_BASE+0x2C` (`EN7523_AXI_CONFIG`) | clear bit 4 |
| `0x1EFBE7F0` | `MCUCFG_BASE+0x7F0` (`EN7523_L2C_CONFIG`) | `[11:8]=0`, bit0=1 (128K L2 + 128K SRAM) |
| `0x1EFBE640` | `MCUCFG_BASE+0x640` | if SCU strap bit0: `[4:0]=0x12` |
| `0x1EFBE7C0` | `MCUCFG_BASE+0x7C0` | if SCU strap bit0: set `[10:9]` |
| `0x1FB0009C` | SCU | strap select (test bit 0) |
| `0x1FB00958` | SCU | hand-off flag before relocation (`=1`) |
| `0x1E800000..0x1E860000` | `ECNT_NPU_SRAM_BASE` (384 KB) | self-test `5555/AAAA` then clear |
| `0x1F000000` | front-end SRAM/TZRAM | relocation destination |
| `0x1FA10004/14/18/20/24/28/2C/30/34/38/3C/40/44` | SPI controller | flash read protocol (see §4) |
| `0x1FA10114` | `SPI_CONTROLLER_REGS_STRAP` | boot-flash select (bit0 / bit1) |
| `0x08000000` | `ECNT_L2_SRAM_BASE` (128 KB) | next-stage load target |

## 4. SPI controller protocol (0x1FA10000)

| off | name | dir | meaning |
|-----|------|-----|---------|
| `0x04` | CTRL | W | clear status |
| `0x14` | MODE | W | `9` |
| `0x18` | READY | R | wait `==0` |
| `0x20` | START | W | `1` |
| `0x24` | DONE | R | wait `==1` after TRIG |
| `0x28` | CMD | W | `((op & 0x1F) << 9) \| (mode & 0x1FF)` |
| `0x2C` | BUSY | R | wait `==0` |
| `0x30` | TRIG | W | `1` |
| `0x34` | TX_RDY | R | wait `==0` |
| `0x38` | TX_DATA | W | byte out |
| `0x3C` | RX_RDY | R | wait `==0` |
| `0x40` | RX_ACK | W | `1` |
| `0x44` | RX_DATA | R | byte in (low 8 bits) |

`op` values (from `ecnt_spi_controller.h`): `0x00` CSH, `0x08` OUTS, `0x0C` INS,
`0x0E` IND. The `0x374` state machine issues READ (`0x03`) with a 3-byte address.

## 5. CP15 / system register sequence (blob 0x040..0x110)

Verified to reassemble **byte-identically** with GNU `as`:

| instr | blob offset | encoding |
|-------|-------------|----------|
| `mcr p15,0,r0,c1,c0,0` (SCTLR=0x00C50838) | `0x44` | `ee010f10` |
| `cps #0x16` (Monitor) | `0x4C` | `f1020016` |
| `mcr p15,0,r0,c12,c0,0/1` (VBAR/MVBAR=0x7C0) | `0x58`,`0x5C` | `ee0c0f10`/`ee0c0f30` |
| SCTLR \|= 0x1002 (A,I) | `0x74` | `ee010f10` |
| `mcr p15,0,r0,c1,c1,0` (SCR.SIF=0x200) | `0x80` | `ee010f11` |
| `mcr p15,0,r0,c1,c1,2` (NSACR, \|=0xC00 CP10/11) | `0x98` | `ee010f51` |
| `mcr p15,0,r0,c1,c0,2` (`0x0F00000`) | `0xA4` | `ee010f50` |
| `mcr p15,0,r0,c1,c3,1` (`0x00808000`) | `0xB0` | `ee010f33` |
| `mcr p15,0,r0,c9,c12,0` (PMCR=0xE0) | `0xB8` | `ee090f1c` |
| `mrc p15,0,r0,c0,c1,0` (ID_PFR0 probe) | `0xBC` | `ee100f11` |
| `mrrc/mcrr p15,0,r0,r1,c15` | `0x15C`/`0x164` | `ec510f0f`/`ec410f0f` |
| `mrrc/mcrr p15,1,r0,r1,c15` | `0x1B8`/`0x1C0` | `ec510f1f`/`ec410f1f` |

## 6. Deliberate deviations from the blob

1. **SP set earlier** – the blob sets `sp = 0x1E824000` (NPU SRAM) right before
   relocation, i.e. *after* the NPU-SRAM test/clear. The open BL1 needs a stack
   for its C code, so it sets `sp = 0x1E904000` (top of NPU SRAM2,
   `ECNT_NPU_SRAM2_BASE`) at the very start — a region that is **not** cleared.
2. **`memset`/`memcpy` no-ops dropped** – the blob calls them with length `0`.
3. **SPI loader simplified** – a clean READ (`0x03`) path is implemented instead
   of the blob's combined NOR/NAND + READ-ID state machine. The register
   protocol is identical; only the *sequence* is cleaner.
4. **Relocation uses symbols** – `_bl1_image_end` / `_bl1_resume` instead of the
   hard-coded `0x7E0` / `0x2F0`.

Everything else (CP15, MCUCFG, SRAM test, SCU hand-off, load addresses
`0xC00 -> 0x08000000`, magic `0xAA640001`) is transcribed verbatim.

## 7. Build / verify

```sh
cd plat/ecnt/en7523/bl1
make                      # -> bl1.bin (size checked <= 2048)
make size dump            # size report / disassembly
# or via the top-level script:
BL1_MODE=source SOC=en7523 ../..../build.sh all
```

Current size: **2016 bytes** (same as the vendor blob, fits the 2 KB window).

**Not yet done:** functional boot test on hardware. The loader semantics
(`strap bit1`, magic `0xAA640001`, `0xC00` offset) are transcribed from the blob
but have not been validated against real silicon.
