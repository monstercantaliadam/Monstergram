import json
import os
import pathlib
import subprocess
import sys


SCHEMA = 1
OLD_SOURCE_TIME = 946684800
PATCH_PATH = "Telegram/build/patches/lib_ui_colors.patch"


def tracked_files(root):
    output = subprocess.check_output(
        ["git", "-C", str(root), "ls-files", "--recurse-submodules", "--stage", "-z"]
    )
    files = {}
    for record in output.split(b"\0"):
        if not record:
            continue
        metadata, path = record.split(b"\t", 1)
        mode, object_id, stage = metadata.split()
        if mode == b"160000" or stage != b"0":
            continue
        name = path.decode("utf-8", "surrogateescape")
        files[name] = object_id.decode("ascii")
    return files


def main():
    action, root_name = sys.argv[1:]
    root = pathlib.Path(root_name).resolve()
    manifest = root / "out" / ".monstergram-source-manifest.json"
    current = tracked_files(root)

    if action == "save":
        manifest.parent.mkdir(parents=True, exist_ok=True)
        manifest.write_text(
            json.dumps({"schema": SCHEMA, "files": current}, sort_keys=True),
            encoding="utf-8",
        )
        print(f"Recorded {len(current)} tracked source files for the next build.")
        return

    if action != "restore":
        raise ValueError(f"Unknown action: {action}")
    if not manifest.is_file():
        print("No previous build cache; the first build compiles all sources.")
        return

    previous = json.loads(manifest.read_text(encoding="utf-8"))
    if previous.get("schema") != SCHEMA or not isinstance(previous.get("files"), dict):
        raise ValueError("Unsupported build cache manifest")

    old = previous["files"]
    patch_changed = old.get(PATCH_PATH) != current.get(PATCH_PATH)
    unchanged = 0
    changed = 0
    for name, object_id in current.items():
        if old.get(name) != object_id or (patch_changed and name.startswith("Telegram/lib_ui/")):
            changed += 1
            continue
        source = root / pathlib.Path(name.replace("/", os.sep))
        if source.is_file() and not source.is_symlink():
            os.utime(source, (OLD_SOURCE_TIME, OLD_SOURCE_TIME))
            unchanged += 1

    print(f"Restored timestamps for {unchanged} unchanged files; {changed} changed files remain new.")


if __name__ == "__main__":
    main()
