# Overlay V2 — phase 1: read-only dependency reporter

**Experimental only.** This is a separate Windows-host proof of concept, not a
replacement for DREL 4.0, LINK99 3.9.59 or the TMS99105 OVLMGR. No main branch,
runtime, executable, object format or native SBC tool has been modified.

## Implemented

The Python 3.9+ reporter reads one-module R99 objects, including EPOINT,
PSIZE, AORG_MARK and the existing XREFS extension:

    EXT "XREFS"
    XCHAIN count,symbol
    CHAIN location
    ...

A JSON manifest assigns modules to logical overlay groups. The report
identifies exports, reference sites and cross-group references requiring a
resident dispatch gate.

- Report-only (default): reports dependencies and unknown export owners.
- Enforcement (--enforce): rejects direct references from resident to overlay
  or between two different overlay groups.

IMPORTANT: The existing XREFS format does not distinguish function calls,
function pointers or data references. Cross-group references are potential
unsafe references, not proven function calls. The tool does not generate gates.
Any unknown external owner is a warning; a partial manifest cannot certify
the safety of an executable.

## Example manifest: groups.json

{
  "schema": "overlay-v2-report/1",
  "groups": [
    {"name": "resident", "kind": "resident", "modules": ["CC_RESIDENT.R99"]},
    {"name": "expr", "kind": "overlay",
     "modules": ["CC_EXPR_A.R99", "CC_EXPR_B.R99"]},
    {"name": "decl", "kind": "overlay", "modules": ["CC_DECL.R99"]}
  ]
}

This example is intentionally partial, not a validated production SMALLC99
manifest. Paths resolve relative to --root.

## Usage (Windows PowerShell)

    python overlay_v2/ovlcheck.py groups.json --root build_tms
    python overlay_v2/ovlcheck.py groups.json --root build_tms --json
    python overlay_v2/ovlcheck.py groups.json --root build_tms --enforce

Run the regression suite:

    cd overlay_v2
    python -m unittest -v

The suite covers synthetic REL99 bit-stream parsing, group and reference
classification, duplicate exports, malformed input, group size limits,
manifest validation and strict failure. It also parses two genuine R99
objects from SMALLC99 main commit 95b4412d, verifying expected exports and
references. Those tests verify the reader, NOT runtime execution.

## Limitations

- This code is NOT integrated into LINK99 or DREL and modifies no executable.
- Only one module per R99 file is supported, not .LIB or legacy chain formats.
- Not all existing binaries emit AORG_MARK; a missing mark is not proof of
  relocatability.
- A 16 KiB group-size check is an approximate size budget only, not a layout.
- No physical page or virtual segment allocation, segment collision detection,
  link resolution, call-site typing, call gates or function-pointer support.
- No pointer-lifetime analysis, runtime restoration proof or indirect calls.
- References to resident exports and within one logical group are merely
  group-compatible; hardware/runtime correctness remains unproven.
- Unresolved exports outside the supplied manifest remain explicitly unknown.

## Next steps (not implemented)

1. Validate dependency reporting against the complete build's actual objects.
2. Define one authoritative logical group manifest for linker planning.
3. Investigate versioned EXT or sidecar call-site metadata without allocating
   a new REL99 record type; unsupported metadata must fail safely.
4. Prototype placement and generated call gates in an isolated assembly harness.
5. Consider SMALLC99 integration only after independent runtime verification.

The Windows toolchain main, SMALLC99 main and native SBC tools remain out of
scope for source changes at this stage.


## Real SMALLC99 compatibility finding (read-only evidence)

The current SMALLC99 resident bridge module exports names also exported by
overlay implementations: constexpr and expression (CC_EXPR_A), test
(CC_EXPR_B), and statement (CC_STMT). The resident definitions forward via
the R_* trampoline entries. The present LINK99 symbols are sorted, and its
newsym() inserts equal names after earlier entries; link() walks that table.
The actual target therefore depends on link input order, not just symbol
name or logical group. This is a design constraint for the V2 resolver.

The phase-1 checker deliberately marks duplicate exports as errors; it is
not yet capable of reproducing LINK99 resolution semantics. DO NOT use this
prototype to reject a working SMALLC99 build. A future version must record
ordered module definitions and distinguish resident bridge symbols from
overlay implementation entry points before classifying those references.
A naive name-only graph produces false cross-overlay warnings.

Observed against SMALLC99 main 95b4412d; no production changes were made.
