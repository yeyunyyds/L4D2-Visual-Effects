#!/usr/bin/env python3
"""Experimental fail-closed ABI diagnostic package, never a production release."""
from pathlib import Path
import subprocess
import zipfile
import json

root=Path(__file__).resolve().parents[1]
build=root/'build'
dll=build/'L4D2Visual.dll'
if not dll.is_file():
    raise SystemExit("Build Windows x86 DLL first")
filename='L4D2Visual-FirstPersonBody-Experimental.zip'
source_name='L4D2Visual-FirstPersonBody-Experimental-source.zip'
revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
install={
 'bin/neko/plugins/L4D2Visual.dll':dll,
 'bin/neko/plugins/L4D2Visual/hud.json':root/'defaults/hud.default.json',
 'bin/neko/plugins/L4D2Visual/images/README.txt':root/'docs/wall-pictures-folder.txt',
 'bin/neko/plugins/L4D2Visual/licenses/LICENSE':root/'LICENSE',
 'bin/neko/plugins/L4D2Visual/licenses/NOTICE.md':root/'NOTICE.md',
}
for path in sorted((root/'vendor').rglob('*')):
    if path.is_file() and ('license' in path.name.lower() or 'copying' in path.name.lower()):
        install['bin/neko/plugins/L4D2Visual/licenses/vendor/'+path.relative_to(root/'vendor').as_posix()]=path
readme=(root/'docs/INSTALL.md').read_text(encoding='utf-8')
readme+='\n\n# First-person body experiment -- NOT WORKING YET\n\n'
readme+='This is a diagnostic-only ABI probe. The first-person body menu and preference\n'
readme+='are present, but the native ShouldDraw entry has NOT been verified. Enabling\n'
readme+='the menu option does NOT make the survivor visible. No visibility hook is\n'
readme+='installed. This is not a production release. Game-side testing is NOT done.\n'
readme+='See docs/FIRST-PERSON-BODY-ABI-BLOCKER.md in the source ZIP for details.\n'
readme+='Install only with the game closed; back up your DLL and hud.json first.\n'
readme+='To roll back, restore the backed-up DLL/hud.json and restart the game.\n'
readme+='\nSource commit: '+revision+'\n'
with zipfile.ZipFile(build/filename,'w',zipfile.ZIP_DEFLATED) as z:
    for target, source in install.items():
        z.write(source,target)
    z.writestr('README.md',readme)
with (build/source_name).open('wb') as f:
    subprocess.run(['git','archive','--format=zip','HEAD'],cwd=root,check=True,stdout=f)
with zipfile.ZipFile(build/source_name,'a') as z:
    z.comment=revision.encode('ascii')
for name in (filename,source_name):
    with zipfile.ZipFile(build/name) as z:
        assert z.testzip() is None
with zipfile.ZipFile(build/filename) as z:
    assert {p.split('/')[0] for p in z.namelist()}=={'bin','README.md'}
    data=z.read('bin/neko/plugins/L4D2Visual.dll')
    pe=int.from_bytes(data[0x3c:0x40],'little')
    assert data[pe:pe+4]==b'PE\x00\x00'
    assert int.from_bytes(data[pe+4:pe+6],'little')==0x14c
    assert b'0.8.0-fpb-abi-probe' in data
    config=json.loads(z.read('bin/neko/plugins/L4D2Visual/hud.json'))
    assert config['firstPersonBody']['enabled'] is False
print(build/filename)
print(build/source_name)
