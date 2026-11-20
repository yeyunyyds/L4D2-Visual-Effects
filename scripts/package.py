"""Build an install-only ZIP without separate license/notice files.

Redistribution notices and full license terms are appended to the existing
README.md, so extracting bin/ only installs the plugin and its config.
"""
from pathlib import Path
import subprocess
import zipfile

root = Path(__file__).resolve().parents[1]
binary = root / "build/L4D2Visual.dll"
if not binary.is_file():
    raise SystemExit("Build the DLL first")
output = root / "build/L4D2Visual-0.8.0.zip"

files = {
    "bin/neko/plugins/L4D2Visual/hud.json": root / "defaults/hud.default.json",
    "bin/neko/plugins/L4D2Visual.dll": binary,
}

# Keep full license texts distributed with the binary, without adding license
# files (or a licenses/ folder) to the installed game directory.
notices = [
    ("GNU General Public License v3", root / "LICENSE"),
    ("Third-party attributions and provenance", root / "NOTICE.md"),
    ("L4D2 Bridge / shared HUD MIT terms", root / "vendor/BRIDGE-LICENSE.MIT"),
    ("nlohmann/json MIT terms", root / "vendor/JSON-LICENSE.MIT"),
    ("Dear ImGui MIT terms", root / "vendor/imgui/LICENSE.txt"),
]

readme = (root / "docs/INSTALL.md").read_text(encoding="utf-8")
try:
    revision = subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=root, text=True
    ).strip()
except (OSError, subprocess.CalledProcessError):
    revision = "source archive"

readme += "\nSource commit: " + revision + "\n"
readme += "\n---\n\n# 版权声明和许可证原文 / Legal notices and licenses\n"
readme += (
    "\nThe full licenses and third-party attribution are preserved here "
    "rather than installed as separate files under bin/.\n"
)
for heading, source in notices:
    # A missing license must fail packaging rather than silently dropping it.
    content = source.read_text(encoding="utf-8").rstrip()
    readme += f"\n## {heading}\n\n\x60\x60\x60text\n{content}\n\x60\x60\x60\n"

with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED) as archive:
    for target, source in files.items():
        archive.write(source, target)
    archive.writestr("README.md", readme)

with zipfile.ZipFile(output) as archive:
    expected = set(files) | {"README.md"}
    names = set(archive.namelist())
    assert names == expected, f"Unexpected ZIP entries: {names ^ expected}"
    assert archive.testzip() is None
    bundled_readme = archive.read("README.md").decode("utf-8")
    assert all(source.read_text(encoding="utf-8").rstrip() in bundled_readme
               for _, source in notices)
print(output)
