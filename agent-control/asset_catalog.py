#!/usr/bin/env python3
"""Read-only Quake III PK3 asset catalog. Never extracts or modifies game data."""
import os
import zipfile
from pathlib import Path, PurePosixPath

ASSET_ROOT = Path(os.environ.get("Q3_ASSET_ROOT", "./baseq3")).expanduser().resolve()
MAX_ARCHIVES = 64
MAX_ENTRIES_PER_ARCHIVE = 200000
MODEL_EXTENSIONS = {".md3", ".mdc"}
TEXTURE_EXTENSIONS = {".tga", ".jpg", ".jpeg", ".png"}


def _member_parts(name):
    normalized = name.replace("\\", "/")
    path = PurePosixPath(normalized)
    if path.is_absolute() or any(part in ("", ".", "..") for part in path.parts):
        return None
    return tuple(part.lower() for part in path.parts)


def _new_model(name):
    return {"name": name, "md3": [], "skins": [], "has_animation_cfg": False,
            "textures": 0, "tags": []}


def scan_archive(archive_path):
    result = {"archive": archive_path.name, "players": {}, "weapons": {},
              "mapobjects": {}, "maps": [], "shaders": 0, "textures": 0,
              "errors": []}
    try:
        with zipfile.ZipFile(str(archive_path), "r") as archive:
            entries = archive.infolist()
            if len(entries) > MAX_ENTRIES_PER_ARCHIVE:
                result["errors"].append("Archive entry limit exceeded; archive skipped.")
                return result
            for entry in entries:
                if entry.is_dir():
                    continue
                parts = _member_parts(entry.filename)
                if not parts:
                    continue
                suffix = Path(parts[-1]).suffix.lower()
                if suffix in TEXTURE_EXTENSIONS:
                    result["textures"] += 1
                if suffix == ".shader":
                    result["shaders"] += 1
                if len(parts) == 2 and parts[0] == "maps" and suffix == ".bsp":
                    result["maps"].append(Path(parts[-1]).stem)
                    continue
                if len(parts) < 3 or parts[0] != "models":
                    continue
                group = parts[1]
                if group in ("players", "weapons2", "mapobjects"):
                    name = parts[2]
                    collection = {"players": result["players"],
                                  "weapons2": result["weapons"],
                                  "mapobjects": result["mapobjects"]}[group]
                    model = collection.setdefault(name, _new_model(name))
                    rel = "/".join(parts[3:])
                    if suffix in MODEL_EXTENSIONS:
                        model["md3"].append(rel)
                    elif suffix == ".skin":
                        model["skins"].append(rel)
                    elif parts[-1] == "animation.cfg":
                        model["has_animation_cfg"] = True
                    elif suffix in TEXTURE_EXTENSIONS:
                        model["textures"] += 1
                    elif suffix == ".md3" and "tag_" in parts[-1]:
                        model["tags"].append(parts[-1])
    except (OSError, zipfile.BadZipFile, RuntimeError) as error:
        result["errors"].append("%s: %s" % (archive_path.name, str(error)[:200]))
    return result


def build_catalog(root=None):
    asset_root = Path(root).expanduser().resolve() if root else ASSET_ROOT
    catalog = {"asset_root": str(asset_root), "archives": [], "players": {},
               "weapons": {}, "mapobjects": {}, "maps": [], "shaders": 0,
               "textures": 0, "errors": []}
    if not asset_root.is_dir():
        catalog["errors"].append(
            "Asset directory not found. Set Q3_ASSET_ROOT to the folder containing your .pk3 files.")
        return catalog
    archives = sorted(asset_root.glob("*.pk3"))[:MAX_ARCHIVES]
    for archive_path in archives:
        item = scan_archive(archive_path)
        catalog["archives"].append(item["archive"])
        catalog["errors"].extend(item["errors"])
        catalog["shaders"] += item["shaders"]
        catalog["textures"] += item["textures"]
        for key in ("players", "weapons", "mapobjects"):
            for name, model in item[key].items():
                existing = catalog[key].setdefault(name, _new_model(name))
                for field in ("md3", "skins", "tags"):
                    existing[field].extend(value for value in model[field]
                                           if value not in existing[field])
                existing["has_animation_cfg"] |= model["has_animation_cfg"]
                existing["textures"] += model["textures"]
        catalog["maps"].extend(item["maps"])
    for key in ("players", "weapons", "mapobjects"):
        catalog[key] = [catalog[key][name] for name in sorted(catalog[key])]
    catalog["maps"] = sorted(set(catalog["maps"]))
    catalog["counts"] = {
        "archives": len(catalog["archives"]),
        "players": len(catalog["players"]),
        "weapons": len(catalog["weapons"]),
        "mapobjects": len(catalog["mapobjects"]),
        "maps": len(catalog["maps"]),
        "shaders": catalog["shaders"],
        "textures": catalog["textures"],
    }
    return catalog
