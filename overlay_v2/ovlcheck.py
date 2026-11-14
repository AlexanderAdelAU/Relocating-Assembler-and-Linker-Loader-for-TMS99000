#!/usr/bin/env python3
"""Read-only REL99 overlay-dependency report (experimental Overlay V2, phase 1).

No linker output is modified. XREFS identifies references, not call sites;
a cross-overlay reference is a candidate for a call gate, not proof that
it is a function call. --enforce rejects cross-group direct references.
"""

import argparse
import json
from dataclasses import dataclass, field
from pathlib import Path
import sys


class RelError(ValueError):
    pass


class Bits:
    def __init__(self, data):
        self.data = data
        self.pos = 0

    def read(self, count):
        if self.pos + count > len(self.data) * 8:
            raise RelError(f"truncated REL99 stream at bit {self.pos}")
        result = 0
        for _ in range(count):
            byte = self.data[self.pos // 8]
            result = (result << 1) | ((byte >> (7 - self.pos % 8)) & 1)
            self.pos += 1
        return result

    def align(self):
        self.pos = (self.pos + 7) & ~7

    def word(self):
        lo = self.read(8)
        hi = self.read(8)
        return (hi << 8) | lo

    def symbol(self):
        length = self.read(4)
        return bytes(self.read(8) for _ in range(length)).decode("ascii", "replace")

    def record(self):
        if self.read(1) == 0:
            return (0, 0, self.read(8), "")
        primary = self.read(2)
        if primary:
            return (primary, primary, self.word(), "")
        item = self.read(4) + 4
        kind, value, name = 0, 0, ""
        if item in (4, 5, 6, 7, 8):
            name = self.symbol()
        elif item in (9, 10, 11):
            kind, value, name = self.read(2), self.word(), self.symbol()
        elif item in (12, 13, 14, 15, 16, 17, 18):
            kind, value = self.read(2), self.word()
        if item in (18, 19):
            self.align()
        return (item, kind, value, name)


@dataclass
class Module:
    path: str
    module_name: str = ""
    exports: dict = field(default_factory=dict)
    references: list = field(default_factory=list)
    aorg: list = field(default_factory=list)
    psize: int = 0


def parse_r99(path):
    """Read a single-module R99 object; reject ambiguous or truncated inputs."""
    reader = Bits(Path(path).read_bytes())
    module = Module(str(path))
    seen_eprog = False
    xref_name = None
    xref_left = 0
    marker = False
    while True:
        item, kind, value, name = reader.record()
        if item == 6:  # PNAME
            if module.module_name:
                raise RelError(f"{path}: multiple modules per file not supported in phase 1")
            module.module_name = name
        elif item == 8:  # EXT extension marker
            if name == "XREFS":
                marker = True
        elif item == 10:  # XCHAIN: XREFS count and target name
            if not marker or xref_left:
                raise RelError(f"{path}: unsupported or nested external-chain encoding")
            xref_name = name
            xref_left = value
            marker = False
        elif item == 16:  # CHAIN: XREFS reference location, or AORG marker
            if xref_left:
                module.references.append((xref_name, value))
                xref_left -= 1
            else:
                module.aorg.append(value)
        elif item == 11:  # EPOINT
            if name in module.exports:
                raise RelError(f"{path}: duplicate export {name}")
            module.exports[name] = value
        elif item == 17:  # PSIZE
            module.psize = value
        elif item == 18:  # EPROG
            if xref_left:
                raise RelError(f"{path}: incomplete XREFS group")
            seen_eprog = True
        elif item == 19:  # EFILE
            if not module.module_name or not seen_eprog:
                raise RelError(f"{path}: missing PNAME or EPROG")
            return module


def assess(manifest, root, enforce=False):
    """Report symbol dependencies; do not inspect/modify output executable."""
    if manifest.get("schema") != "overlay-v2-report/1":
        raise RelError("manifest schema must be overlay-v2-report/1")
    groups = manifest.get("groups")
    if not isinstance(groups, list) or not groups:
        raise RelError("manifest groups must be a nonempty array")
    modules = {}
    group_by_module = {}
    seen_group_names = set()
    errors = []
    for group in groups:
        name, kind = group.get("name"), group.get("kind")
        if not name or kind not in ("resident", "overlay"):
            raise RelError("group requires name and kind=resident|overlay")
        if name in seen_group_names:
            raise RelError(f"duplicate group name: {name}")
        seen_group_names.add(name)
        for relative in group.get("modules", []):
            if relative in group_by_module:
                raise RelError(f"module {relative} belongs to multiple groups")
            obj = (Path(root) / relative).resolve()
            base = Path(root).resolve()
            if not obj.is_relative_to(base):
                raise RelError(f"module path escapes root: {relative}")
            try:
                module = parse_r99(obj)
            except (OSError, RelError) as exc:
                raise RelError(f"cannot parse {relative}: {exc}") from exc
            modules[relative] = module
            group_by_module[relative] = (name, kind)
    exports = {}
    for path, module in modules.items():
        for symbol in module.exports:
            if symbol in exports:
                errors.append(f"DUPLICATE_EXPORT {symbol}: {exports[symbol]} and {path}")
            else:
                exports[symbol] = path
    dependencies = []
    warnings = []
    for path, module in modules.items():
        from_group, from_kind = group_by_module[path]
        for symbol, offset in module.references:
            dest = exports.get(symbol)
            if dest is None:
                status = "UNRESOLVED_OR_EXTERNAL_LIBRARY"
                warnings.append(f"{path}: {symbol} at >{offset:04X} has no export among declared modules")
                to_group = None
            else:
                to_group, to_kind = group_by_module[dest]
                if from_group == to_group or to_kind == "resident":
                    status = "DIRECT_SAFE_BY_GROUP"
                else:
                    status = "CROSS_OVERLAY_GATE_REQUIRED"
                    if enforce:
                        errors.append(f"CROSS_OVERLAY_REFERENCE {path} -> {dest}: {symbol} at >{offset:04X}")
            dependencies.append({"from": path, "from_group": from_group,
                                 "symbol": symbol, "at": f">{offset:04X}",
                                 "to": dest, "to_group": to_group, "status": status})
    for group in groups:
        if group["kind"] != "overlay":
            continue
        total = sum(modules[relative].psize for relative in group.get("modules", []))
        if total > 0x4000:
            errors.append(f"OVERLAY_GROUP_TOO_LARGE {group['name']}: {total} > 16384")
    return {"schema": "overlay-v2-report/1", "modules": [
        {"file": path, "r99_name": m.module_name, "group": group_by_module[path][0],
         "bytes": m.psize, "aorg": [f">{x:04X}" for x in m.aorg],
         "exports": sorted(m.exports)} for path, m in modules.items()],
        "references": dependencies, "warnings": warnings, "errors": errors,
        "enforce": enforce, "result": "FAIL" if errors else "PASS"}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path, help="JSON grouping manifest")
    parser.add_argument("--root", type=Path, default=Path("."), help="R99 file root")
    parser.add_argument("--enforce", action="store_true", help="fail on cross-overlay references")
    parser.add_argument("--json", action="store_true", help="emit machine-readable report")
    args = parser.parse_args(argv)
    try:
        report = assess(json.loads(args.manifest.read_text(encoding="utf-8")), args.root, args.enforce)
    except (OSError, ValueError, KeyError, TypeError) as exc:
        print(f"OVLV2 ERROR: {exc}", file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print("OVLV2 PHASE 1: READ-ONLY DEPENDENCY REPORT")
        for m in report["modules"]:
            print(f"  {m['file']} group={m['group']} size={m['bytes']} exports={','.join(m['exports'])}")
        for ref in report["references"]:
            print(f"  {ref['status']}: {ref['from']}:{ref['at']} -> {ref['symbol']} ({ref['to'] or '?'})")
        for msg in report["warnings"]:
            print(f"  WARNING: {msg}")
        for msg in report["errors"]:
            print(f"  ERROR: {msg}")
        print(f"RESULT: {report['result']}")
    return 1 if report["errors"] else 0


if __name__ == "__main__":
    sys.exit(main())
