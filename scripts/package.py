"""Package the test DLL and its complete corresponding source; no game assets."""
from pathlib import Path
import zipfile
root = Path(__file__).resolve().parents[1]
binary = root / 'build/L4D2Visual.dll'
if not binary.is_file():
    raise SystemExit('Build the DLL first')
output = root / 'build/L4D2Visual-0.7.0-test.zip'
with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED) as archive:
    archive.write(binary, 'bin/neko/plugins/L4D2Visual.dll')
    archive.write(root / 'README.md', 'README.md')
    archive.write(root / 'docs/wall-pictures-folder.txt', 'bin/neko/plugins/L4D2Visual/images/README.txt')
    for name in ('src', 'vendor', 'tests', 'scripts', 'docs'):
        for path in sorted((root / name).rglob('*')):
            if path.is_file() and '__pycache__' not in path.parts:
                archive.write(path, 'source/' + path.relative_to(root).as_posix())
    for name in ('README.md', 'LICENSE', 'NOTICE.md', '.gitignore'):
        archive.write(root / name, 'source/' + name)
print(output)
