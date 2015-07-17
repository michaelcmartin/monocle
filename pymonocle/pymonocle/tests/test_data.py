import os
import tempfile
from unittest import TestCase


RESFILE_JSON = """{
"data": {
    "null": null,
    "number": 42,
    "string": "hello",
    "list_of_strings": ["line 1", "line 2", "line 3"],
    "list_of_lists": [[1, 2], [3, 4]]
}
}"""


class TemporaryResourceDir(object):
    def __init__(self):
        self.path = tempfile.mkdtemp()
        self.files = []

    def cleanup(self):
        for f in self.files:
            f.close()
        os.rmdir(self.path)

    def add_file(self, content, suffix=''):
        f = tempfile.NamedTemporaryFile(dir=self.path, suffix=suffix)
        self.files.append(f)
        f.write(content)
        f.flush()
        return os.path.basename(f.name)


class TestMonocleData(TestCase):
    def setUp(self):
        import pymonocle
        self.mncl = pymonocle
        self.resdir = TemporaryResourceDir()
        self.mncl.add_resource_directory(self.resdir.path)

    def tearDown(self):
        self.mncl.unload_all_resources()
        self.resdir.cleanup()

    def setup_resource_map(self, data_json):
        content = '{ "data": %s }' % (data_json,)
        filename = self.resdir.add_file(content, suffix='.json')
        self.mncl.load_resmap(filename)
        print open(os.path.join(self.resdir.path, filename)).read()

    def test_null(self):
        self.setup_resource_map('{"null": null}')
        self.assertEqual(self.mncl.data_resource("null"), None)

    def test_number(self):
        self.setup_resource_map('{"number": 42}')
        self.assertEqual(self.mncl.data_resource("number"), 42)

    def test_string(self):
        self.setup_resource_map('{"string": "hello"}')
        self.assertEqual(self.mncl.data_resource("string"), "hello")

    def test_list_of_strings(self):
        self.setup_resource_map('{"list_of_strings": ["line 1", "line 2"]}')
        self.assertEqual(
            self.mncl.data_resource("list_of_strings"), ["line 1", "line 2"])

    def test_list_of_lists(self):
        self.setup_resource_map('{"list_of_lists": [[1, 2], [3, 4]]}')
        self.assertEqual(
            self.mncl.data_resource("list_of_lists"), [[1, 2], [3, 4]])

    def test_dict(self):
        self.setup_resource_map('{"dict": {"a": 1, "b": "z"}}')
        self.assertEqual(self.mncl.data_resource("dict"), {"a": 1, "b": "z"})

    def test_complex(self):
        self.setup_resource_map('{"complex": [{"a": {"b": [1, 2, null]}}]}')
        self.assertEqual(
            self.mncl.data_resource("complex"), [{"a": {"b": [1, 2, None]}}])
