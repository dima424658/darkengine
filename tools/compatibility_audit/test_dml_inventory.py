import unittest
from dml_inventory import commands


class InventoryTests(unittest.TestCase):
    def test_nested_blocks_and_comments(self):
        text = '''DML1
!FINGERPRINT { GAM "test.gam" }
// ++Link ignored
+ObjProp "Object" "Scripts"
{
   "Script 0" "a // b { }"
}
/* Create ignored */
Create "Parent" "Child" { +ObjProp "Name" = "Value" }
#script "example"
'''
        self.assertEqual(dict(commands(text)), {
            "!fingerprint": 1, "+objprop": 1, "create": 1, "#script": 1})

    def test_scalar_and_prefixes(self):
        self.assertEqual(dict(commands('DML1\n-ObjProp 3 "X"\n++Link 1 2 "ControlDevice"')),
                         {"-objprop": 1, "++link": 1})

    def test_raw_field_trailing_quote(self):
        self.assertEqual(dict(commands('DML1\n+ObjProp 1 "PhysAttr" {\n"Rest Axes" +X Axis"\n}')),
                         {"+objprop": 1})

    def test_malformed_files(self):
        for text in [' DML1', 'DML10', 'DML1\n}', 'DML1\nCreate 1 {',
                     'DML1\n/*', 'DML1\nObjName 1 "unfinished']:
            with self.subTest(text=text), self.assertRaises(ValueError):
                commands(text)


if __name__ == "__main__":
    unittest.main()
