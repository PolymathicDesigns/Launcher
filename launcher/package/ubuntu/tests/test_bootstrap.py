import io
import os
from pathlib import Path
import subprocess
import tarfile
import tempfile
import unittest

WRAPPER = Path(__file__).resolve().parents[1] / "multimc/opt/multimc/run.sh"

class BootstrapTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        commands = self.root / "commands"
        commands.mkdir()
        self.archive = self.root / "archive.tar.gz"
        self.installed = self.root / "data with spaces" / "multimc"
        self.env = dict(os.environ, PATH=str(commands) + os.pathsep + os.environ["PATH"],
                        XDG_DATA_HOME=str(self.installed.parent), TEST_ARCHIVE=str(self.archive))
        scripts = {
            "wget": '#!/bin/sh\n[ "$1" = "-O" ] || exit 2\ncp "$TEST_ARCHIVE" "$2" || exit 3\nexit "${TEST_DOWNLOAD_STATUS:-0}"\n',
            "zenity": '#!/bin/sh\ncat >/dev/null\n',
        }
        for name, script in scripts.items():
            path = commands / name
            path.write_text(script)
            path.chmod(0o755)
        self.make_archive()

    def make_archive(self, include_binary=True):
        entries = {"MultiMC/MultiMC": b'#!/bin/sh\nprintf "%s\\n" "$@" > launched\n'}
        if include_binary:
            entries["MultiMC/bin/MultiMC"] = b'#!/bin/sh\nexit 0\n'
        with tarfile.open(self.archive, "w:gz") as archive:
            for name, data in entries.items():
                info = tarfile.TarInfo(name)
                info.size = len(data)
                info.mode = 0o755
                archive.addfile(info, io.BytesIO(data))

    def run_wrapper(self):
        return subprocess.run(["bash", str(WRAPPER), "argument with spaces"], env=self.env,
                              capture_output=True, text=True, timeout=10)

    def assert_failed_cleanly(self, result):
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse((self.installed / ".bootstrap-complete").exists())
        self.assertFalse((self.installed / "launched").exists())
        self.assertEqual(list(self.installed.glob(".bootstrap.*")), [])

    def test_success_and_reuse(self):
        result = self.run_wrapper()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual((self.installed / "launched").read_text(), "argument with spaces\n")
        self.archive.unlink()
        self.assertEqual(self.run_wrapper().returncode, 0)

    def test_download_failure_preserves_user_data(self):
        self.installed.mkdir(parents=True)
        accounts = self.installed / "accounts.json"
        accounts.write_text("existing data")
        self.env["TEST_DOWNLOAD_STATUS"] = "8"
        self.assert_failed_cleanly(self.run_wrapper())
        self.assertEqual(accounts.read_text(), "existing data")

    def test_corrupt_archive_can_retry(self):
        self.archive.write_bytes(b"not an archive")
        self.assert_failed_cleanly(self.run_wrapper())
        self.make_archive()
        self.assertEqual(self.run_wrapper().returncode, 0)

    def test_incomplete_package_can_retry(self):
        self.make_archive(include_binary=False)
        self.assert_failed_cleanly(self.run_wrapper())
        self.make_archive()
        self.assertEqual(self.run_wrapper().returncode, 0)

    def test_partial_install_is_repaired(self):
        self.installed.mkdir(parents=True)
        launcher = self.installed / "MultiMC"
        launcher.write_text("#!/bin/sh\nexit 73\n")
        launcher.chmod(0o755)
        result = self.run_wrapper()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue((self.installed / ".bootstrap-complete").exists())

if __name__ == "__main__":
    unittest.main()
