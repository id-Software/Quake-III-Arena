import os
import tempfile
import unittest
import zipfile
from pathlib import Path

from asset_catalog import build_catalog, scan_archive


class AssetCatalogTests(unittest.TestCase):
    def test_catalog_finds_all_asset_categories_without_extracting(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            archive_path = root / "pak0.pk3"
            with zipfile.ZipFile(archive_path, "w") as archive:
                archive.writestr("models/players/sarge/lower.md3", b"")
                archive.writestr("models/players/sarge/animation.cfg", b"")
                archive.writestr("models/players/sarge/default.skin", b"")
                archive.writestr("models/weapons2/rocketl/rocket.md3", b"")
                archive.writestr("models/mapobjects/columns/column.md3", b"")
                archive.writestr("maps/q3dm1.bsp", b"")
                archive.writestr("scripts/base.shader", b"")
            result = build_catalog(root)
            self.assertEqual(result["counts"]["players"], 1)
            self.assertEqual(result["counts"]["weapons"], 1)
            self.assertEqual(result["counts"]["mapobjects"], 1)
            self.assertEqual(result["maps"], ["q3dm1"])
            self.assertTrue(result["players"][0]["has_animation_cfg"])
            self.assertFalse((root / "models").exists())

    def test_unsafe_zip_paths_are_ignored(self):
        with tempfile.TemporaryDirectory() as temp:
            archive_path = Path(temp) / "bad.pk3"
            with zipfile.ZipFile(archive_path, "w") as archive:
                archive.writestr("../models/players/evil/body.md3", b"")
                archive.writestr("/models/players/evil/body.md3", b"")
            result = scan_archive(archive_path)
            self.assertEqual(result["players"], {})


if __name__ == "__main__":
    unittest.main()
