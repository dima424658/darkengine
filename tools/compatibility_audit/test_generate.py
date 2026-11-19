import tempfile
import unittest
from pathlib import Path
from generate import requirements


class RequirementTests(unittest.TestCase):
    def test_material_headings_exclude_prose(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "material-format.txt"
            path.write_text("material files reside nearby\n\nterrain_scale <int>\n-------------------\n"
                            "    explanation\n\nillum_map <float> <texture>\n---------------------------\n")
            self.assertEqual(list(requirements(path)), ["terrain_scale", "illum_map"])

    def test_service_declarations(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "script" / "Engine.txt"
            path.parent.mkdir()
            path.write_text("STDMETHOD_(BOOL, ConfigIsDefined)(const char *name) PURE;\n"
                            "STDMETHOD(SetTagBlock)(const char *name) PURE;\n")
            self.assertEqual(list(requirements(path)), ["ConfigIsDefined", "SetTagBlock"])


if __name__ == "__main__":
    unittest.main()
