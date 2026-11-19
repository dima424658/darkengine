import unittest
from script_contracts import contracts


class ContractTests(unittest.TestCase):
    def test_overloads_multiline_defaults_and_conditional_branches(self):
        text = '''DECLARE_SCRIPT_SERVICE(Test, 1)
{
#ifndef THIEF1
STDMETHOD(Set)(object obj, const char *name) PURE;
#else
#if defined(SHOCK)
STDMETHOD(Set)(object obj,
 const char *name, const char *value = "a//b") PURE;
#elif defined(OTHER)
STDMETHOD(Get)() PURE;
#else
STDMETHOD(Remove)() PURE;
#endif
#endif
};
'''
        items = list(contracts(text, True))
        self.assertEqual([item.service for item in items], ['Test'] * 4)
        self.assertEqual(items[1].line, 7)
        self.assertIn('"a//b"', items[1].declaration)
        self.assertEqual(items[1].guards, ('#ifndef THIEF1 -> #else', '#if defined(SHOCK)'))
        self.assertEqual(items[3].guards[1], '#if defined(SHOCK) -> #elif defined(OTHER) -> #else')

    def test_comment_examples_are_not_declarations_and_notes_do_not_leak(self):
        text = '''// **** Available only in API version 3+ ****
First
{
// Pretend();
/* Fake(); */
int Real();
// **** Available only in API version 12+ ****
int Added();
}
Second
{
int Base();
}
'''
        items = list(contracts(text, False))
        self.assertEqual(len(items), 3)
        self.assertIn('version 3+', items[0].version_note)
        self.assertIn('version 12+', items[1].version_note)
        self.assertEqual(items[2].version_note, '')

    def test_native_version_annotation_without_game_prefix(self):
        items = list(contracts('DECLARE_SCRIPT_SERVICE(Test, 1)\n{\n'
                               '// new functions in v1.28:\nSTDMETHOD(New)() PURE;\n};', True))
        self.assertIn('v1.28', items[0].version_note)

    def test_native_interfaces_and_spaced_macros(self):
        text = ('DECLARE_INTERFACE_(IScriptMan, IUnknown)\n{\n'
                'STDMETHOD (GameInit)(THIS) PURE;\n};\n'
                'DECLARE_INTERFACE(IDarkOverlayHandler)\n{\n'
                'STDMETHOD_(void, DrawHUD)(THIS) PURE;\n};\n')
        items = list(contracts(text, True))
        self.assertEqual([item.service for item in items],
                         ['IScriptMan', 'IDarkOverlayHandler'])
        self.assertIn('GameInit', items[0].declaration)

    def test_incomplete_input_is_rejected(self):
        for text in ['#else\n', '#ifdef X\n',
                     'DECLARE_SCRIPT_SERVICE(Test, 1)\n{\nSTDMETHOD(Set)(\n']:
            with self.subTest(text=text), self.assertRaises(ValueError):
                list(contracts(text, True))


if __name__ == '__main__':
    unittest.main()
