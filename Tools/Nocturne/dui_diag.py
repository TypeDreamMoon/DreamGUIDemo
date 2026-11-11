# Prints the .dui compiler's diagnostics for the files whose path contains any of the arguments.
import io
import json
import os
import sys

path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "DUI", ".dui-diagnostics.json")
data = json.load(io.open(path, encoding="utf-8-sig"))
wanted = sys.argv[1:]
for name, entry in data.get("files", {}).items():
    if wanted and not any(w in name for w in wanted):
        continue
    diags = entry.get("diagnostics", [])
    print("%s  (%d)" % (name.split("/DUI/")[-1], len(diags)))
    for d in diags[:40]:
        print("  %s DUI%04d %d:%d %s" % (d.get("severity", "?")[0].upper(), d.get("code", 0), d.get("line", 0), d.get("column", 0), d.get("message", "")))
