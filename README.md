# link99 — TMS9900 Relocatable Linker

## Overview

`link99` is a relocatable cross linker for the TMS9900 architecture, producing `.COM` and `.EXE` binaries for the TMS99105 SBC. It processes `.R99` relocatable object modules produced by the `r99` cross-assembler, resolves external references, and generates flat or paged chain format output binaries that will run on a TMS99000 family processor.

**Authors:**
- J.E. Hendrix — original Small-MAC linker (8080/CP/M, 1985)
- Alex Cameron — TMS9900 port (1984), Eclipse cross-compiler (2015), ongoing development

**Current version:** 3.10.3

---

## Tools in this repository

This repository holds the Windows **cross** toolchain for the TMS9900/TMS99105. These tools run on a PC and produce code for the SBC; they are used to build programs such as [SMALLC99](https://github.com/AlexanderAdelAU/SmallC99-TMS99105-Native-C-Compiler) and the [native SBC tools](https://github.com/AlexanderAdelAU/TMS99105-Native-Toolchain).

| Folder | Tool | What it does |
|---|---|---|
| `r99/` | **r99** | Relocatable cross-assembler: `.A99` source → `.R99` object module |
| `link99/` | **link99** | Linker: `.R99` modules and libraries → `.COM` or paged `.EXE` (documented below) |
| `drel/` | **drel** | Dumps and lints `.R99`/`.LIB` files, run by hand. Version 5.0 removed `-i` (overlay include files): `link99 -Y` builds the overlay table and thunks |
| `Includes/`, `cfunctions/`, `rel99/` | | Shared headers, Small-C run-time functions and the REL file reader/writer used by the tools |

A typical build is: compile C to assembly with `smallcp` (from [Small-C-Plus-for-TMS9900-99000](https://github.com/AlexanderAdelAU/Small-C-Plus-for-TMS9900-99000)), assemble with `r99`, then link with `link99`.

---

## r99 — TMS9900 Relocatable Cross-Assembler

`r99` assembles TMS9900/TMS99105 assembly source (`.A99`) into a relocatable object module (`.R99`) in the Microsoft REL-style format that `link99` reads. It is based on William C. Colley III's 1980 9900 cross-assembler, extended by Alex Cameron for relocation and the TMS99105.

### Usage

```
r99 PROG [options]
```

`PROG` is the source name without extension. The options are a single word of letter pairs, in the CP/M style of the original:

| Option | Meaning |
|---|---|
| `S`*d* | Read the source `PROG.A99` from drive *d* (`A`–`D`), or `S-` for the current drive |
| `H`*d* | Write the object module `PROG.R99` to drive *d*, or `H-` for the current drive |
| `L`*d* | Write a listing `PROG.L99` to drive *d*, or `L-` for the current drive; `LX` lists to the console |

Example — assemble `PROG.A99` on drive C and write `PROG.R99`, as the SMALLC99 build does:

```
r99 PROG SCHC
link99 PROG.COM PROG.R99
```

### Source language

- The full TMS9900 instruction set, plus the TMS99105 additions (`BIND`, `BLSK`, `DIVS`, `MPYS`, `LST`, `LWP`, `TMB`, `TCMB`, `TSMB`).
- Directives: `AORG`, `ORG`, `EQU`, `SET`, `BSS`, `BYTE`, `WORD`, `TEXT`, `EVEN`, `DXOP`, `IF`/`ELSE`/`ENDI`, `INCL`, `NAM`, `END`.
- Linking: `ENT symbol` makes a symbol visible to other modules; `EXT symbol` refers to a symbol in another module.
- Symbols may be up to 15 characters, matching the REL format limit.

### Building

`r99` is K&R C built as a 32-bit Windows program with MinGW, together with the shared `Includes/` headers and the `cfunctions/` run-time:

```
gcc -I Includes -o r99.exe r99\*.c cfunctions\*.c
```

It stores pointers in `int`s, so build it as 32-bit (it runs fine on 64-bit Windows).

---

## Usage

```
link99 [-B] [-M] [-O#] [-G#] [-D#] [-S] [-P#] [-PAGES#-#] [-Ymap] program [module/library ...]
```

### Arguments

| Argument | Description |
|----------|-------------|
| `program` | Output filename. If a matching `.R99` exists it is loaded first. |
| `module` | Relocatable object module (`.R99`) |
| `library` | Library file (`.LIB`) — searched multi-pass |

### Switches

| Switch | Description |
|--------|-------------|
| `-M` | Monitor — verbose output showing module loads, PREL values, symbol resolutions, chain blocks |
| `-O#` | Set code base address to hex `#` (e.g. `-O1000` for EXE files loading at TPA) |
| `-P#` | Tag the next module as physical page `#` (0–15). Used for overlay modules. |
| `-PAGES#-#` | Auto-assign pages in range — e.g. `-PAGES2-15` assigns next free page to each module |
| `-Ymap` | OVL99 overlay map (3.10.0). Pages, overlay table and thunks come from the map — see [Overlay map](#overlay-map--y) |
| `-B` | Big program — force all code to disk from the start, freeing buffer for symbol table |
| `-G#` | Make program absolute at hex address `#`; write as `.LGO` instead of `.COM` |
| `-D#` | Set absolute data segment base to hex address `#` |
| `-S` | Accepted for compatibility; it currently has no effect (the flag is read but not used) |

---

## Overlay map (`-Y`)

From 3.10.0 the linker builds an application's overlay glue itself, as
2.11BSD's `ovld` did. One text file, the overlay map, declares the
overlays; nothing in the application's source or build script repeats it.

The native SBC LINK99 (in [TMS99105-Native-Toolchain](https://github.com/AlexanderAdelAU/TMS99105-Native-Toolchain/tree/main/link99))
reads the same map and, given the same arguments, writes the same bytes; it
refuses page spans (`lo-hi`). The runtime side of OVL99 (thunks, `OVCALL`,
`OVLMGR`, the rules) is described in
[OVL99.md](https://github.com/AlexanderAdelAU/TMS99105-Native-Toolchain/blob/main/OVL99.md).

```
link99 -O1000 -M -S -YOVLMAP.TXT APP.EXE  STARTUP.R99 MAIN.R99 OVLMGR.R99 OVCALL.R99 ...  OVLA.R99 OVLB.R99  CLIB99.LIB
```

The overlay modules are listed **without** `-P`; their pages come from the
map. `OVLMGR` refers to the table with `EXT OVL_TABLE` and includes no
generated file.

### Map format

One line per overlay module. `#` starts a comment.

```
# module   page   overlay  entries
OVLHELLO   4      GREET    hello greet
OVLGRET2   6      GREET    shout
OVLBYE     5      BYE      farewell count
```

| Field | Meaning |
|---|---|
| `module` | Base name of an object file on the command line (matched without path or extension). |
| `page` | Physical page 1–15, or a span `lo-hi` for a module larger than 4K. Pages are per segment: no (segment, page) pair may be used twice, but page 3 behind segment 8 and page 3 behind segment 9 are different memory (3.10.3). |
| `overlay` | Modules with the same name are one overlay: one ID, mapped together. IDs are given in order of first appearance. |
| `entries` | The functions anything outside the module may call, or `-` for none. |

An entry takes one of three forms:

| Form | Thunk | The routine |
|---|---|---|
| `foo` | a thunk named `foo` | renamed `~foo` in its module, so every outside call binds to the thunk |
| `R_EXPR=expression` | a thunk named `R_EXPR` reaching `expression` | keeps its name; direct calls inside its overlay are untouched. Several public names may share one routine |
| either, suffixed `:tail` | maps the overlay and **branches** — no OVCALL frame | for a top-level entry called from resident code that needs nothing mapped back |

Two optional lines:

| Line | Meaning |
|---|---|
| `window 8-n` | Overlays may use segments 8..n only (default `8-11`, all of >8000–BFFF). FileEdit says `window 8`: its document banks are at >9000–BFFF. |
| `guest NAME` | Overlay `NAME` maps part of the window **without** segment 8, so it never becomes the window owner (SMALLC99's CLI at >A000). Safe only with an OVCALL that always takes a frame. Its entries cannot be `:tail`. |

The virtual segment of a module is its own `AORG`, never the map.

### What the linker does with it

1. **Pages.** Each map module is linked on its map page, as `-P` would.
2. **Renaming.** A plain entry `foo` is renamed `~foo` as its module loads. For an alias the linker adds a private `~routine` for *that module's* entry point, because another module may define a routine of the same name (SMALLC99's resident `statement`, `test`, `expression`…). Thunks always target `~routine`.
3. **`OVLGLUE.R99`.** Just before the first overlay module the linker writes and loads one resident module. It holds `OVL_TABLE`, with 10-word rows of five (segment,page) pairs, row = ID×20 and row 0 unused. It also holds one 12-byte thunk per public name:

   ```
   framed:  LI R1,id / LI R2,~routine / B @OVCALL
   :tail:   LI R1,id / CALL @OVLMGR   / B @~routine
   ```

   It is loaded before the overlays because a resident module's address is its place in the link buffer. The table is filled in once every module is in. The file is left on disk for inspection with `drel`.
4. **Checks**, each a FATAL with the reason:
   - every map module is on the command line and has an AORG, and every entry is exported by its module;
   - every overlay maps segment 8 first and once (a guest never maps it);
   - segments are contiguous, inside the window, at most five pages;
   - no (segment, page) pair is used twice, and no module has both `-P` and a map line.

With `-M` the listing shows every row and binding. Without `-Y` the
output is byte-identical to 3.9.59.

---

## Output File Formats

### Flat Binary — `.COM`

Used for programs that run entirely in segment 0 (TPA at `0x1000`). No header.

```
[raw binary bytes, loaded directly to TPA]
```

### Paged Chain Format — `.EXE`

Used for programs with overlay modules assembled with `AORG` and tagged with `-P#`. The shell's `LOADERCODE_EXE` reads the chain to program the mapper and copy each block.

Each block:
```
next_offset   word    byte distance to next block header (0 = last block)
page          word    physical page number (0 = common/flat)
start         word    virtual load address
size          word    byte count of data
[data bytes]
```

Blocks are chained via `next_offset`. The loader programs the mapper for each paged block, copies the data to the virtual address, and launches from the `start` address of the first block.

### EXE Build Command

```
link99 -O1000 -M ovltest.EXE ovltest.R99 ovlmgr.R99 -P2 ovla.R99 -P3 ovlb.R99
```

- `-O1000` sets `cbase=0x1000` for base relocatable modules (TPA for EXE files)
- Base modules (`ovltest.R99`, `ovlmgr.R99`) are placed contiguously from `0x1000`
- `-P2 ovla.R99` tags OVLA as physical page 2; `AORG 2000H` in source sets virtual address
- `-P3 ovlb.R99` tags OVLB as physical page 3
- Output has one page-0 block (base program) + one block per overlay page

---

## Linking Multiple Modules

Modules are linked in command-line order. External references (`EXT`) in one module are resolved against entry points (`ENT`) in subsequent modules.

### COM flat program

```
link99 -M ovltest.COM ovltest.R99 ovlmgr.R99
```

Both modules are relocatable — placed contiguously from `cbase=0x1000`.

### EXE with overlays

```
link99 -O1000 -M ovltest.EXE ovltest.R99 -P2 ovla.R99 -P3 ovlb.R99
```

Note: with the overlay manager (`OVLMGR`) now resident in common at `0x0A00H` (booted by the shell), it no longer needs to be linked with application EXEs.

### Overlay modules as separate COM files (manual load via `LOAD` command)

```
link99 -M -P2 ovla.COM ovla.R99
link99 -M -P3 ovlb.COM ovlb.R99
```

These produce single-block chain format `.COM` files loadable via the shell `LOAD` command.

### Libraries

```
link99 program.COM main.R99 clib99.LIB iolib99.LIB
```

Libraries are searched **multi-pass** — after all command-line arguments are processed, all libraries are re-searched repeatedly until no new modules are loaded. This resolves cross-library dependencies regardless of library order.

### 32-bit arithmetic: `LCALL.R99`

`iolib99/src/call.A99` holds the helpers compiled code calls for 16-bit
arithmetic (`_ccmult`, `_ccdiv` …); `CALL.R99` is linked into every program.
`iolib99/src/lcall.A99` is its 32-bit companion, linked only by programs that
use 32-bit values:

```
link99 -O1000 program.COM startup.R99 main.R99 call.R99 lcall.R99 clib99.LIB
```

- A 32-bit value is two words, high word first: `int v[2]`.
- C-callable, result first: `ladd(r,a,b)`, `lsub`, `lmul`, `ldiv`, `lmod`,
  `ludiv`, `lumod`, `lneg`, `lshl`/`lshr`/`lsar`, `lcmp`/`lucmp`, `lcpy`,
  `lset(r,hi,lo)`, `itol`, `utol`, `ltoi`, `lmul16` (16×16→32),
  `ltod`/`ltou`/`ltox` (text) and `dtol` (from text). The header of
  `lcall.A99` lists them all.
- Register helpers for compiled code, entered by `BL` like `_ccmult`: value in
  `R4:R5`, operand in `R2:R3` — `_cladd`, `_clsub`, `_clmul`, `_cldiv`,
  `_cludiv`, `_clneg`, `_clcmp`, `_clucmp`, `_clshl`, `_clshr`, `_clsar`.
  A compiler with a `long` type calls these; the C functions are wrappers
  round them.
- 1,008 bytes. Tested by `link99/tests/lcall` in the TMS99105-Native-Toolchain
  repository (1,504 checks).

---

## REL File Format

The `.R99` file is a **packed bit-stream**, not a byte stream. Each item is encoded as a type tag followed by data:

| Type | Bits | Description |
|------|------|-------------|
| `ABS` | `0` + 8 data bits | Absolute byte — copied unchanged |
| `PREL` | `1 01` + 16 bits | Code-relative word — `cbase + cmod + field` at link time |
| `DREL` | `1 10` + 16 bits | Data-relative word |
| `ENAME` | special | Module name record |
| `PSIZE` | special | Code segment size |
| `DSIZE` | special | Data segment size |
| `EPOINT` | special + name | Entry point declaration (`ENT`) |
| `XCHAIN` | special + name | External reference chain (`EXT`) |
| `SETLC` | special + addr | Set location counter (e.g. for BSS) |
| `EPROG` | special | End of module |

### PREL Relocation

When the linker loads a module, each PREL word is computed as:

```
absolute = cbase + cmod + field
```

Where:
- `cbase` = program base address (set by `-O#`, default `0` in paged mode)
- `cmod` = offset of this module within the linked image
- `field` = the raw PREL value from the REL file (module-relative offset)

**Important:** `field=0` is a valid PREL value (symbol at offset 0 of module) and always has `cmod` added. The linker never skips zero-field PRels.

### AORG Modules

Overlay modules use `AORG` to assemble at an absolute virtual address (e.g. `AORG 2000H`). Their PREL values are already absolute (`>= PAGE_SEG = 0x1000`). The linker detects this and does **not** add `cbase` or `cmod` — doing so would double-relocate them. The distinction is:

- `field < 0x1000` → relocatable module → add `cbase + cmod`
- `field >= 0x1000` → AORG module → add `cmod` only (already absolute within page)

### cmod Calculation

`cmod` is saved at `ENAME` time (start of module), before any `SETLC` records move the location counter. This ensures that modules containing BSS data have the correct `cmod` value.

---

## External Reference Resolution

After each module loads, `resolve()` walks all unresolved external chains:

1. Finds the `XCHAIN` linked list for the external symbol in the code image
2. Patches each location with the absolute address of the matching `EPOINT`
3. For relocatable modules: `xbuf = xrloc - cbase` (convert virtual addr to buffer offset)
4. For AORG modules: same — `cbase` is stripped to get the correct buffer position

---

## Monitor Output (`-M`)

The `-M` flag produces verbose output useful for debugging:

```
Loading module: OVLTEST
  136 Code Bytes at  0' 1000 OVLTEST
  PREL seen: field= 78 pagemode=YES pmmin_valid=NO
  ...
  AORG fixup: base=1000 end=1136

Loading module: OVLA
   58 Code Bytes at 172' 1172 OVLA
  PREL seen: field=008 pagemode=YES pmmin_valid=NO
  ...
  AORG fixup: base=2000 end=2058

Phase 2 - Writing execution files

  CHAIN BLOCK pg= 0 start=1000 size=  172
  CHAIN BLOCK pg= 2 start=2000 size=   58
  CHAIN BLOCK pg= 3 start=2000 size=   58
```

Key fields:
- `field=0` with `cmod > 0` — symbol at start of module, must get `cmod` added
- `pagemode=YES` — paged/AORG module detected
- `AORG fixup: base=2000 end=2058` — AORG base detected and handled
- `CHAIN BLOCK` — output block summary showing page, virtual start, and size

---

## Version History

| Version | Change |
|---------|--------|
| 1.0 | J.E. Hendrix — original Small-MAC linker (8080/CP/M) |
| 2.0 | TMS9900 port |
| 3.0 | Eclipse cross-compiler version |
| 3.1 | COMBASE moved to 0x500 for extended memory |
| 3.2 | Disk overflow fully implemented |
| 3.3 | First positional arg may be output name only |
| 3.4 | Relocation tables moved to disk (.CT$/.DT$) |
| 3.5 | Entry-index scheme for CT$/DT$; multi-pass library search |
| 3.6 | `-P#` page tagging; pagemap table; 6116 mapper support |
| 3.7 | Transparent mapping via GAL22V10; trampolines eliminated |
| 3.8 | Page-separated output blocks; pagemap header format |
| 3.9 | Cleaner command syntax |
| 3.9.1 | 512KB buffer; paged AORG fixup; flat raw binary output |
| 3.9.2 | Fix `resolve()` direct patch for assembly externals |
| 3.9.3 | Fix `resolve()` — `xrloc` is virtual addr, subtract `cbase` for buffer access |
| 3.9.4 | Fix `cmod` — save `cloc` at ENAME before SETLC records corrupt it |
| 3.9.5 | Fix `pmmin_global` — only track first module PRels for AORG detection |
| 3.9.6 | Fix PREL — always add `cmod` even when `field=0` (offset 0 in module) |
| 3.9.7 | Chain block format output replacing pagemap sentinel format |
| 3.9.8 | `-O#` switch — set `cbase` explicitly for EXE builds |
| 3.9.9 | Suppress ABS/PREL/DREL/CREL items from `-M` monitor output |
| 3.9.10 | Fix PREL/cbase for mixed AORG/relocatable builds with `-O#`: phase 1 skips `cmod` add for AORG modules; phase 2 skips `cbase` add for AORG modules; `resolve()` subtracts `cbase` from `xrloc` for correct buffer offset |
| … | 3.9.11–3.9.58: see the history at the top of `link99/link99.c` |
| 3.9.59 | XPOFF marker told apart from a relocatable word at buffer offset >FFFE |
| 3.10.0 | `-Y` overlay map: pages, `OVL_TABLE` and thunks built by the linker; aliases, `:tail`, `guest`, `window`. An external reference now adds the addend left in the word (`@SYM+2` against an EXT) |
| 3.10.1 | Reference-slot lint (formerly `drel -c`): every external reference must be an ABS word its module emitted, claimed by one symbol; FATAL otherwise. Builds no longer need DREL. Output unchanged |
| 3.10.2 | FATAL if a resident module ends at or above the lowest overlay address (a library member loaded after the overlays is placed above them by their size). Output unchanged |
| 3.10.3 | Pages are per segment: paged blocks in different segments are never merged, `-Y` checks (segment, page) pairs, `window 8-n` may reach D. A paged block whose sector padding would cross its segment carries its true size, so an overlay may use its whole 4K page (the old >0FC0 limit) |

---

## Building

`link99` is K&R C, built on Windows with MinGW `gcc`, together with the shared headers, run-time functions and REL reader in this repository:

```
gcc -I Includes -o link99.exe link99\link99.c rel99\GETREL.c rel99\PUTREL.c rel99\SEEREL.c cfunctions\*.c
```

| Folder | Provides |
|---|---|
| `link99/link99.c` | The linker |
| `rel99/` | `GETREL.c` (the packed bit-stream reader for `.R99` files), `PUTREL.c`, `SEEREL.c` |
| `cfunctions/` | Small-C run-time helpers (`itox`, `utoi`, `getarg`, `extend` …) |
| `Includes/` | `rel99.h`, `stdio.h`, `clib.h`, `fcntl.h` and the REL format headers |

---

## Common Issues

**Unresolved externals:**
- Ensure `ENT symbol` is in the module providing the symbol
- Ensure `EXT symbol` is in the module using it
- Check symbol name truncation — names are limited to 15 characters (`MAXSYM` in `rel99.h`)

**Wrong address for symbol at module offset 0:**
- Fixed in v3.9.6 — `field=0` PRels were not getting `cmod` added

**SETLC corrupting cmod:**
- Fixed in v3.9.4 — BSS data generates SETLC records that moved `cloc` before PSIZE fired
- `cmod` is now saved at ENAME time

**AORG module addresses wrong under `-O#`:**
- Fixed in v3.9.10 — AORG modules (field >= `PAGE_SEG`) now correctly skip `cbase` addition
- Affects builds mixing relocatable base modules with AORG overlay modules

**EXE file hanging on launch:**
- Verify `-O1000` is used so base modules load at TPA (`0x1000`)
- Check that overlay modules have `AORG 2000H` (or correct virtual address)
- Do not include OVLMGR in the link command — it is now resident in common at `0x0A00H`
