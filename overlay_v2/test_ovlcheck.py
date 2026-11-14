"""Synthetic REL99 regression tests and two real SMALLC99 R99 object samples.

They do NOT prove runtime behaviour or validate linker-generated executables.
"""
import base64
from contextlib import redirect_stdout
from io import StringIO
import json
import tempfile
import unittest
from pathlib import Path

from ovlcheck import RelError, parse_r99, assess, main


class RelWriter:
    def __init__(self):
        self.bits = []

    def put(self, value, width):
        assert 0 <= value < 1 << width
        self.bits.extend((value >> i) & 1 for i in reversed(range(width)))

    def word(self, value):
        self.put(value & 255, 8)
        self.put(value >> 8, 8)

    def symbol(self, value):
        data = value.encode("ascii")
        assert len(data) <= 15
        self.put(len(data), 4)
        for ch in data:
            self.put(ch, 8)

    def record(self, item, value=0, name="", kind=1):
        if item == 0:
            self.put(0, 1)
            self.put(value, 8)
        elif item in (1, 2, 3):
            self.put(1, 1)
            self.put(item, 2)
            self.word(value)
        else:
            self.put(1, 1)
            self.put(0, 2)
            self.put(item - 4, 4)
            if item in (4, 5, 6, 7, 8):
                self.symbol(name)
            elif item in (9, 10, 11):
                self.put(kind, 2)
                self.word(value)
                self.symbol(name)
            elif item in (12, 13, 14, 15, 16, 17, 18):
                self.put(kind, 2)
                self.word(value)
            if item in (18, 19):
                self.bits += [0] * ((-len(self.bits)) % 8)

    def bytes(self):
        self.bits += [0] * ((-len(self.bits)) % 8)
        return bytes(sum(self.bits[i + j] << (7 - j) for j in range(8))
                     for i in range(0, len(self.bits), 8))


def module_bytes(name, exports=(), refs=(), size=0x120, aorg=0x8000):
    w = RelWriter()
    w.record(6, name=name)
    w.record(17, size)
    if aorg is not None:
        w.record(16, aorg)
    for symbol, addr in exports:
        w.record(11, addr, symbol)
    for symbol, site in refs:
        w.record(8, name="XREFS")
        w.record(10, 1, symbol, kind=0)
        w.record(16, site)
    w.record(18, 0, kind=0)
    w.record(19)
    return w.bytes()


class OverlayV2Tests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.manifest = {"schema": "overlay-v2-report/1", "groups": [
            {"name": "resident", "kind": "resident", "modules": ["res.R99"]},
            {"name": "expr", "kind": "overlay", "modules": ["expr.R99", "expr2.R99"]},
            {"name": "decl", "kind": "overlay", "modules": ["decl.R99"]},
        ]}
        (self.root / "res.R99").write_bytes(module_bytes("RES", [("common", 0x1010)], [], aorg=None))
        (self.root / "expr.R99").write_bytes(module_bytes("EXPR", [("calculate", 0x8010)], [("common", 0x8020)]))
        (self.root / "expr2.R99").write_bytes(module_bytes("EXP2", [("level1", 0x9010)], [("calculate", 0x9030)], aorg=0x9000))
        (self.root / "decl.R99").write_bytes(module_bytes("DECL", [("do_decl", 0x8010)], [("calculate", 0x8020)]))

    def test_parses_reference_location_and_exports(self):
        m = parse_r99(self.root / "decl.R99")
        self.assertEqual(m.module_name, "DECL")
        self.assertEqual(m.exports, {"do_decl": 0x8010})
        self.assertEqual(m.references, [("calculate", 0x8020)])
        self.assertEqual(m.aorg, [0x8000])

    def test_same_group_and_resident_references_compatible(self):
        report = assess(self.manifest, self.root)
        x = {(r["from"], r["symbol"]): r["status"] for r in report["references"]}
        self.assertEqual(x[("expr.R99", "common")], "DIRECT_SAFE_BY_GROUP")
        self.assertEqual(x[("expr2.R99", "calculate")], "DIRECT_SAFE_BY_GROUP")
        self.assertEqual(x[("decl.R99", "calculate")], "CROSS_OVERLAY_GATE_REQUIRED")
        self.assertEqual(report["result"], "PASS")  # report-only mode

    def test_enforcement_rejects_cross_group_reference(self):
        report = assess(self.manifest, self.root, enforce=True)
        self.assertEqual(report["result"], "FAIL")
        self.assertTrue(any("CROSS_OVERLAY_REFERENCE decl.R99" in x for x in report["errors"]))
        self.assertFalse(any("CROSS_OVERLAY_REFERENCE expr2.R99" in x for x in report["errors"]))

    def test_duplicate_export_is_error(self):
        (self.root / "expr2.R99").write_bytes(module_bytes("EXP2", [("calculate", 0x9000)]))
        self.assertTrue(any(x.startswith("DUPLICATE_EXPORT") for x in assess(self.manifest, self.root)["errors"]))

    def test_unknown_external_is_reported_as_warning(self):
        (self.root / "decl.R99").write_bytes(module_bytes("DECL", [], [("librarycall", 0x8080)]))
        report = assess(self.manifest, self.root)
        self.assertEqual(report["references"][-1]["status"], "UNRESOLVED_OR_EXTERNAL_LIBRARY")
        self.assertEqual(len(report["warnings"]), 1)

    def test_group_over_16k_is_rejected(self):
        (self.root / "expr.R99").write_bytes(module_bytes("EXP", size=0x3FFF))
        report = assess(self.manifest, self.root)
        self.assertTrue(any("OVERLAY_GROUP_TOO_LARGE expr" in x for x in report["errors"]))

    def test_truncated_object_is_rejected(self):
        (self.root / "decl.R99").write_bytes(b"\x82")
        with self.assertRaises(RelError):
            parse_r99(self.root / "decl.R99")

    def test_duplicate_module_ownership_is_rejected(self):
        self.manifest["groups"][2]["modules"].append("expr.R99")
        with self.assertRaises(RelError):
            assess(self.manifest, self.root)

    def test_duplicate_group_name_is_rejected(self):
        self.manifest["groups"][2]["name"] = "expr"
        with self.assertRaises(RelError):
            assess(self.manifest, self.root)

    def test_root_path_escape_is_rejected(self):
        self.manifest["groups"][2]["modules"] = ["../escape.R99"]
        with self.assertRaises(RelError):
            assess(self.manifest, self.root)

    def test_real_smallc99_r99_objects(self):
        # Blob SHA references on SMALLC99 main commit 95b4412d:
        # CC_PORT.R99 350fde9c3491d2c3503f2c18f22d0a488da03a0a
        # CC_MACS.R99 79772744494c2ecb57258a19fac4ee5c9c5f466c
        samples = {
            "CC_PORT.R99": "hOhoa+oJ6kqQJUE9SVF9JTklUlAAATUKABaoAAABaoAAABawI6AAEqCepKi+kpySqRFWFJFRlOMAIBaerJiajqS+kpySqTECAIirCkioynGAEAhTVFVCSU5JVJiDAE4AAACeGg==",
            "CC_MACS.R99": "hOhoa+moKGpwEbWFjcYCNrCxt0BnB1dG1hY5QAAE1UA6WgEhLZiYlohPAICWYmGAxGCIAAAFCAwMSmMhGiKgABQQhGA+mo7QACWCIAAAGAxAEBAKd0CAwiAgIYKkTzBEAAAAFQjIAgAADRFQAAYIQgtYEdAJBG1hY26OzEwjawsbjHRCeZwdXRtYWOIqwpIqMpxgEAGbWFjcHRymIxPTEunpigT0xRp6cAAAnho="
        }
        for filename, encoded in samples.items():
            (self.root / filename).write_bytes(base64.b64decode(encoded))
        port = parse_r99(self.root / "CC_PORT.R99")
        self.assertEqual(port.exports, {"PORT_INIT": 0})
        self.assertEqual(port.references, [("OVLMGR_INIT", 2), ("STUBINIT", 6)])
        macros = parse_r99(self.root / "CC_MACS.R99")
        self.assertIn("putmac", macros.exports)
        self.assertEqual(macros.psize, 3664)
        self.assertEqual(len(macros.references), 4)

    def test_cli_enforce_exit_status_and_json_report(self):
        f = self.root / "manifest.json"
        f.write_text(json.dumps(self.manifest), encoding="utf-8")
        with redirect_stdout(StringIO()) as printed:
            self.assertEqual(main([str(f), "--root", str(self.root), "--enforce", "--json"]), 1)
        self.assertIn("CROSS_OVERLAY_GATE_REQUIRED", printed.getvalue())


if __name__ == "__main__":
    unittest.main()
