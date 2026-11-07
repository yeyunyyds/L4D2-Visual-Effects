"""Build an install-only archive: bin/ and README.md at the archive root."""
from pathlib import Path
import zipfile

root = Path(__file__).resolve().parents[1]
binary = root / 'build/L4D2Visual.dll'
if not binary.is_file():
    raise SystemExit('Build the DLL first')
output = root / 'build/L4D2Visual-0.7.4.zip'
files = {
    'bin/neko/plugins/L4D2Visual/hud.json': root / 'defaults/hud.default.json',
    'bin/neko/plugins/L4D2Visual.dll': binary,
    'README.md': root / 'docs/INSTALL.md',
    'bin/neko/plugins/L4D2Visual/images/README.txt': root / 'docs/wall-pictures-folder.txt',
}
for name in ('LICENSE', 'NOTICE.md'):
    files['bin/neko/plugins/L4D2Visual/licenses/' + name] = root / name
for path in sorted((root / 'vendor').rglob('*')):
    if path.is_file() and ('license' in path.name.lower() or 'copying' in path.name.lower()):
        files['bin/neko/plugins/L4D2Visual/licenses/vendor/' + path.relative_to(root / 'vendor').as_posix()] = path
with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED) as archive:
    for target, source in files.items():
        archive.write(source, target)
with zipfile.ZipFile(output) as archive:
    assert {name.split('/')[0] for name in archive.namelist()} == {'bin', 'README.md'}
    assert archive.testzip() is None
print(output)
