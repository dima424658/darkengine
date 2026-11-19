import tempfile
import unittest
from pathlib import Path
from generate import requirements, review


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

    def test_squirrel_services_remain_distinct(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "API-reference_services.txt"
            path.write_text("Engine\n{\n    BOOL ConfigIsDefined(string name);\n}\n"
                            "Object\n{\n    ObjID Create(object archetype);\n}\n")
            self.assertEqual(list(requirements(path)), ["Engine.ConfigIsDefined", "Object.Create"])

    def test_dml_syntax_is_not_runtime_support(self):
        path = Path("DOC/dbmod-sample.dml")
        status, _ = review(path, "DML1 syntax", "candidate")
        self.assertEqual(status, "Implemented syntax only; runtime missing")
        status, _ = review(path, "ObjProp", "candidate")
        self.assertEqual(status, "Missing runtime DML behavior")

    def test_rendering_metadata_is_not_runtime_zone_support(self):
        path = Path("DOC/modders_notes.txt")
        status, evidence = review(path, "Ambient Light Zones", "candidate")
        self.assertIn("Missing runtime", status)
        self.assertIn("no callers", evidence)
        status, evidence = review(path, 'Changed Prop: Weather->"Local Weather"', "candidate")
        self.assertIn("legacy", status)
        self.assertIn("boolean fog", evidence)
        status, _ = review(path, "Max View Distance", "candidate")
        self.assertIn("mission minimum missing", status)

    def test_dml_diagnostics_do_not_claim_execution(self):
        path = Path("DOC/dbmod-sample.dml")
        status, evidence = review(path, "Fingerprint PROP/NAME/GAM rules", "candidate")
        self.assertIn("automatic application missing", status)
        self.assertIn("native read-only", evidence)
        status, evidence = review(path, "Object ID remapping", "candidate")
        self.assertIn("lifecycle integration missing", status)
        self.assertIn("live IDs only", evidence)

    def test_overlay_projection_stubs_and_setting_alternatives(self):
        for path, name in [("DOC/script/DarkOverlay.txt", "WorldToScreen"),
                           ("DOC/squirrel_script/API-reference_services.txt", "DarkOverlay.WorldToScreen")]:
            status, evidence = review(Path(path), name, "candidate")
            self.assertIn("Missing projection behavior", status)
            self.assertIn("always returns false", evidence)
        status, evidence = review(Path("DOC/new_config_vars.txt"), "vsync_mode", "candidate")
        self.assertIn("key unsupported", status)
        self.assertIn("not equivalent", evidence)

    def test_service_review_preserves_native_squirrel_distinction(self):
        status, evidence = review(Path("DOC/script/Quest.txt"), "BinGet", "candidate")
        self.assertEqual(status, "Missing native method")
        self.assertIn("Squirrel binary-variable helpers exist separately", evidence)
        status, evidence = review(Path("DOC/script/AnimTexture.txt"), "ChangeTexture", "candidate")
        self.assertEqual(status, "Unverified")
        self.assertEqual(evidence, "candidate")
        status, _ = review(Path("DOC/Quest.txt"), "BinGet", "candidate")
        self.assertEqual(status, "Unverified")


if __name__ == "__main__":
    unittest.main()
