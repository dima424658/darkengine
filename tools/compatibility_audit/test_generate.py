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

    def test_subtitle_settings_and_sample_formats_are_individual_requirements(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "subtitles.txt"
            path.write_text("subtitles_max_dist_<Type> <dist>\n"
                            "    prose\nmovsubtitles_dim\n")
            names = list(requirements(path))
            self.assertIn("subtitles_max_dist_<Type>", names)
            self.assertIn("movsubtitles_dim", names)
            self.assertNotIn("prose", names)
            path = Path(directory) / "font-sample.fon"
            path.write_text("FON1\nresource fonts/serif\n")
            self.assertIn("FON1 header", list(requirements(path)))
            self.assertIn("dynamic", list(requirements(path)))

    def test_resource_loading_gaps_do_not_hide_existing_alternatives(self):
        status, evidence = review(Path("DOC/fm_selector_api.txt"), "SelectFM export and packed ABI", "candidate")
        self.assertIn("configured selection exists", status)
        self.assertIn("no SelectFM", evidence)
        status, evidence = review(Path("DOC/kbd_00000407.ini"), "kbd_00000407", "candidate")
        self.assertIn("native layouts exist", status)
        self.assertIn("compiled tables", evidence)

    def test_audio_support_is_distinct_from_documented_overrides(self):
        status, evidence = review(Path("DOC/al_reverb.ini"), "al_reverb", "candidate")
        self.assertIn("EFX reverb exists", status)
        self.assertIn("compiled", evidence)
        status, evidence = review(Path("DOC/new_config_vars.txt"), "force_underwater_reverb", "candidate")
        self.assertIn("forced/configurable override missing", status)
        self.assertIn("only when room acoustics are unset", evidence)
        status, evidence = review(Path("DOC/new_config_vars.txt"), "snd_decompress_on_load", "candidate")
        self.assertIn("OGG decoding exists", status)
        self.assertIn("without reading", evidence)

    def test_engine_source_review_preserves_runtime_and_canvas_limits(self):
        for path, name in [("DOC/script/Engine.txt", "GetCanvasSize"),
                           ("DOC/squirrel_script/API-reference_services.txt", "Engine.GetCanvasSize")]:
            status, evidence = review(Path(path), name, "candidate")
            self.assertIn("configured resolution instead of actual canvas", status)
            self.assertIn("without querying", evidence)
        status, evidence = review(Path("DOC/script/Engine.txt"), "ConfigGetInt", "candidate")
        self.assertIn("runtime/ABI parity unverified", status)
        self.assertIn("config_get_int", evidence)
        status, _ = review(Path("DOC/script/Engine.txt"), "SetTagBlock", "candidate")
        self.assertEqual(status, "Missing")

    def test_property_inheritance_and_missing_indexed_methods(self):
        status, evidence = review(Path("DOC/script/Property.txt"), "Possessed", "candidate")
        self.assertIn("runtime parity unverified", status)
        self.assertIn("including inheritance", evidence)
        status, evidence = review(Path("DOC/squirrel_script/API-reference_services.txt"), "Property.GetIndexed", "candidate")
        self.assertIn("Missing Squirrel method", status)
        self.assertIn("API 11/12", evidence)
        status, _ = review(Path("DOC/script/Property.txt"), "GetIndexed", "candidate")
        self.assertEqual(status, "Missing native method")

    def test_quest_wildcard_and_binary_helpers_are_not_full_parity(self):
        status, evidence = review(Path("DOC/script/Quest.txt"), "SubscribeMsg", "candidate")
        self.assertIn("wildcard missing", status)
        self.assertIn("literal name", evidence)
        status, _ = review(Path("DOC/squirrel_script/API-reference_services.txt"), "Quest.BinSet", "candidate")
        self.assertIn("campaign persistence missing", status)
        status, evidence = review(Path("DOC/squirrel_script/API-reference_services.txt"), "Quest.BinSetTable", "candidate")
        self.assertIn("Incorrect table API", status)
        self.assertIn("argument 2", evidence)

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
