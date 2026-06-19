# link99 — TMS9900 Relocatable Linker

## Overview

`link99` is a relocatable linker/loader for the TMS9900 architecture, producing `.COM` binaries for the TMS99105 SBC. It processes `.R99` relocatable object modules produced by the `r99` cross-assembler, resolves external references, and generates flat or paged output binaries.

**Authors:**
- J.E. Hendrix — original Small-MAC linker (8080/CP/M, 1985)
- Alex Cameron — TMS9900 port (1984), Eclipse cross-compiler (2015), ongoing development

**Current version:** 3.9.6

---

## Usage

```
link99 [-B] [-M] [-G#] [-D#] [-S] [-P#] [-PAGES#-#] program [module/library ...]
```

### Arguments

| Argument | Description |
|----------|-------------|
| `program` | Output filename (`.COM` appended). If a matching `.R99` exists it is loaded first. |
| `module` | Relocatable object module (`.R99`) |
| `library` | Library file (`.LIB`) — searched multi-pass |

### Switches

| Switch | Description |
|--------|-------------|
| `-M` | Monitor — verbose output showing module loads, PREL values, symbol resolutions |
| `-P#` | Tag the next module as physical page `#` (0–15). Used for overlay modules. |
| `-PAGES#-#` | Auto-assign pages in range — e.g. `-PAGES2-15` assigns next free page to each module |
| `-B` | Big program — force all code to disk from the start, freeing buffer for symbol table |
| `-G#` | Make program absolute at hex address `#`; write as `.LGO` instead of `.COM` |
| `-D#` | Set absolute data segment base to hex address `#` |
| `-S` | Generate Small-C call wrapper to `main()` |

---

## Output File Formats

### Flat Binary (non-paged) — `.COM`

Used for programs that run entirely in segment 0 (TPA). No pagemap header.

```
[raw binary bytes, loaded directly to 0x1000]
```

### Paged Binary — `.COM` with pagemap

Used for overlay modules assembled with `AORG` and tagged with `-P#`. The loader reads the pagemap to program the 6116 mapper registers before copying code to the correct virtual address.

```
FFFF FFFF FFFF              opening sentinel
vstart vend  page           pagemap entry (6 bytes): virtual start, end, physical page
FFFF FFFF FFFF              closing sentinel
size                        block size in bytes
[code bytes...]             code for this page
```

Multiple pagemap entries may appear (one per page used). The loader iterates all entries, programs the mapper for each, then copies the corresponding code block.

---

## Linking Multiple Modules

Modules are linked in command-line order. External references (`EXT`) in one module are resolved against entry points (`ENT`) in subsequent modules.

### Flat program with overlay manager

```
link99 ovltest.COM ovltest.R99 ovlmgr.R99
```

OVLTEST and OVLMGR are both relocatable — the linker places them contiguously from `cbase=0x1000`.

### Overlay modules (paged)

```
link99 -P2 ovla.COM ovla.R99
link99 -P3 ovlb.COM ovlb.R99
```

Each overlay is linked separately. `-P2` tags the module as physical page 2; the linker writes a pagemap entry and places the code at the virtual `AORG` address.

### Libraries

```
link99 program.COM main.R99 clib99.LIB iolib99.LIB
```

Libraries are searched **multi-pass** — after all command-line arguments are processed, all libraries are re-searched repeatedly until no new modules are loaded. This resolves cross-library dependencies regardless of library order on the command line.

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

When the linker loads a module, each PREL word has the absolute address computed as:

```
absolute = cbase + cmod + field
```

Where:
- `cbase` = program base address (default `0x1000` for flat programs)
- `cmod` = offset of this module within the linked image
- `field` = the raw PREL value from the REL file (module-relative offset)

**Important:** `field=0` is a valid PREL value (symbol at offset 0 of module) and always has `cmod` added. The linker never skips zero-field PRels.

### cmod Calculation

`cmod` is saved at `ENAME` time (start of module), before any `SETLC` records can move the location counter. This ensures that modules containing BSS data (which generate `SETLC` records before `PSIZE`) have the correct `cmod` value.

---

## AORG Modules (Overlay Code)

When a module uses `AORG` to assemble at an absolute address (e.g. `AORG 2000H`), the linker detects this automatically via `pmmin_global` — the minimum PREL value seen in the first module. If `pmmin_global >= PAGE_SEG (0x1000)`, the addresses are already absolute and `cbase` is set to 0.

The AORG base is stripped from `SETLC` addresses (which reflect absolute positions) so that module-relative offsets are computed correctly.

---

## External Reference Resolution

After each module loads, `resolve()` walks all unresolved external chains:

1. Finds the `XCHAIN` linked list for the external symbol in the code image
2. Patches each location with the absolute address of the matching `EPOINT`
3. For relocatable modules: `xbuf = xrloc` (buffer offset = location counter value)
4. For AORG modules: `xbuf = xrloc - cbase` (strip the absolute base)

---

## Monitor Output (`-M`)

The `-M` flag produces verbose output useful for debugging linker issues:

```
Loading module: OVLTEST
  12A Code Bytes at  0' 1000 OVLTEST
  PREL seen: field= 76 pagemode=NO pmmin_valid=NO
  ...
Loading module: OVLMGR
   3C Code Bytes at 12A' 112A OVLMGR
  PREL seen: field=130 pagemode=NO pmmin_valid=NO
  PREL seen: field=12A pagemode=NO pmmin_valid=NO   ← offset-0 symbol, cmod added
  ...
Resolving external: OVLMGR     → 0x1132
Resolving external: OVLMGR_INI → 0x1158
Searching for main in cbase=1000 pmmin_global=1E
CODE SIZE  166 (1000-1165)
```

Key fields to watch:
- `field=0` with `cmod > 0` — symbol at start of module, must get `cmod` added
- `pagemode=YES` — paged AORG module
- `AORG fixup: base=2000 end=2062` — linker detected and stripped AORG base

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

---

## Building

`link99` is a standard C program. Build on Windows or Linux:

```bash
gcc -o link99 link99.c getrel.c -I.
```

Or on Windows with MSVC:
```
cl link99.c getrel.c
```

The source includes `getrel.c` which implements the packed bit-stream reader used to decode `.R99` files.

---

## Common Issues

**Unresolved externals:**
- Ensure `ENT symbol` is in the module providing the symbol
- Ensure `EXT symbol` is in the module using it
- Check symbol name truncation — names are limited to 10 characters

**Wrong address for symbol at module offset 0:**
- This was a known bug fixed in v3.9.6 — `field=0` PRels were not getting `cmod` added

**SETLC corrupting cmod:**
- Fixed in v3.9.4 — BSS data generates SETLC records that moved `cloc` before PSIZE fired
- `cmod` is now saved at ENAME time before any SETLC

**AORG module addresses doubling:**
- Fixed in v3.9.5 — `pmmin_global` was being polluted by second-module PRels
- Now only the first module's PRels are tracked for AORG detection
