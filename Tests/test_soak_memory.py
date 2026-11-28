import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location('soak_memory', Path(__file__).parents[1] / 'Tools/soak_memory.py')
memory = importlib.util.module_from_spec(spec)
spec.loader.exec_module(memory)


class SoakMemoryTests(unittest.TestCase):
    def setUp(self):
        self.folder = tempfile.TemporaryDirectory()
        self.addCleanup(self.folder.cleanup)
        self.path = Path(self.folder.name) / 'HostMemory.jsonl'
        self.rows = [dict(wall_since_launch_seconds=i*30, private_bytes=i*1000000,
                          working_set_bytes=500000, virtual_bytes=9000000000,
                          production_verified=i>1) for i in range(1,7)]

    def write(self, rows):
        self.path.write_text('\n'.join(json.dumps(row) for row in rows)+'\n', encoding='utf-8-sig')

    def test_private_growth_and_warmup_are_preserved(self):
        # Stable resident memory must not hide increasing private commitment.
        self.write(self.rows)
        self.assertEqual(memory.read_memory(self.path, True, 180), self.rows)

    def test_new_production_run_cannot_use_legacy_missing_memory(self):
        self.assertIsNone(memory.read_memory(self.path, False, 7200))
        with self.assertRaisesRegex(ValueError, 'missing'):
            memory.read_memory(self.path, True, 7200)

    def test_truncated_or_unverified_run_is_rejected(self):
        self.write(self.rows)
        with self.assertRaisesRegex(ValueError, 'Incomplete'):
            memory.read_memory(self.path, True, 7200)
        for row in self.rows:
            row['production_verified'] = False
        self.write(self.rows)
        with self.assertRaisesRegex(ValueError, 'Incomplete'):
            memory.read_memory(self.path, True, 180)

    def test_invalid_or_reordered_observations_are_rejected(self):
        for value in [float('nan'), float('inf'), 0, -100]:
            with self.subTest(value=value):
                rows = [dict(row) for row in self.rows]
                rows[2]['private_bytes'] = value
                self.write(rows)
                with self.assertRaisesRegex(ValueError, 'Invalid'):
                    memory.read_memory(self.path, True, 180)
        self.write(list(reversed(self.rows)))
        with self.assertRaisesRegex(ValueError, 'Non-monotonic'):
            memory.read_memory(self.path, True, 180)


if __name__ == '__main__':
    unittest.main()
