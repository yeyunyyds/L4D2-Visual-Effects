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
readme=(
 '# L4D2 Visual Effects — experimental first-person body\n\n'
 'This experimental build offers an independently implemented, Evo-style '
 'first-person body presentation. Evo is a visual style reference only. '
 'No Evo source code, binary code, or proprietary assets are copied, '
 'extracted, ported, reused, or included. The projects are not affiliated.\n\n'
 '## Features\n'
 '- Native local-player model display, with verified game-build gating.\n'
 '- Render-only head and third-person arm bone-branch hiding.\n'
 '- Camera-relative dynamic positioning, manual XYZ adjustment, and '
 'per-model calibration.\n'
 '- Experimental GPU height clipping is disabled by default and is not '
 'a verified substitute for model-aware positioning.\n\n'
 '## Current visual preset\n'
 'Body display ON; dynamic anchor ON; camera-yaw follow ON; '
 'head/arm hiding ON; forward=-13, right=-1, up=-1; '
 'GPU clipping OFF. These user-tuned settings prioritize appearance and '
 'do not eliminate all animation or third-party model clipping.\n\n'
 '## Installation and scope\n'
 '1. Exit Left 4 Dead 2 completely. Back up L4D2Visual.dll and hud.json.\n'
 '2. Extract bin/ into the game directory; Windows x86 / L4N v2 required.\n'
 '3. Retain your existing hud.json to keep custom settings. The packaged '
 'sample only provides defaults for a new configuration.\n'
 '4. Open L4D2Visual > First-person Body to adjust and save settings.\n\n'
 'This is NOT a stable release. Game-version identity is validated; '
 'other MODs, camera/animation behavior, and online compatibility '
 'are not guaranteed. Do not assume VAC safety.\n\n'
 'GPL-3.0. See included licenses and NOTICE.md. '
 "Do not replace the game's own DLLs.\n"
)
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
    assert b'0.8.0-fpb-user-preset-experimental' in data
    config=json.loads(z.read('bin/neko/plugins/L4D2Visual/hud.json'))
    assert config['firstPersonBody']['enabled'] is True
    assert config['firstPersonBody']['dynamicAnchor'] is True
    assert config['firstPersonBody']['followCameraYaw'] is True
    assert config['firstPersonBody']['forward'] == -13
    assert config['firstPersonBody']['hideHead'] and config['firstPersonBody']['hideArms']
    assert config['firstPersonBody']['clipEnabled'] is False
print(build/filename)
print(build/source_name)
