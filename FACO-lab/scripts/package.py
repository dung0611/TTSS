"""Package source, documentation, evidence and optional Windows binaries; omit build trees."""
import argparse
import hashlib
import zipfile
from pathlib import Path

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--output", required=True)
    a = ap.parse_args()
    root = Path(__file__).resolve().parents[1]
    output = Path(a.output).resolve()
    files = []
    for path in sorted(root.rglob("*")):
        if not path.is_file():
            continue
        rel = path.relative_to(root)
        if rel.parts[0].startswith("build-") or any(p in {".git", "__pycache__"} for p in rel.parts):
            continue
        if rel.as_posix().startswith("results/pilot/") or path == output or rel.name == "MANIFEST.sha256":
            continue
        files.append(path)
    manifest = "".join(hashlib.sha256(p.read_bytes()).hexdigest() + "  " + p.relative_to(root).as_posix() + "\n" for p in files)
    (root / "MANIFEST.sha256").write_text(manifest, encoding="utf-8")
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED, compresslevel=7) as archive:
        for path in files + [root / "MANIFEST.sha256"]:
            archive.write(path, "FACO-lab/" + path.relative_to(root).as_posix())
    digest = hashlib.sha256(output.read_bytes()).hexdigest()
    output.with_suffix(output.suffix + ".sha256").write_text(digest + "  " + output.name + "\n", encoding="utf-8")
    print(f"Packaged {len(files)+1} files, {output.stat().st_size} bytes: {output}")
    print("SHA256", digest)

if __name__ == "__main__":
    main()
