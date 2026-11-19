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
readme+='\n\n# First-person body: native drawing gate EXPERIMENT\n\n'
readme+='Verified exact client.dll and engine.dll builds are required.\n'\nreadme+='USER-TUNED EXPERIMENTAL DEFAULT: first-person body and dynamic anchor ON.\n'
readme+='One native local-player DrawModel conditional skip is bypassed in memory.\n'
readme+='No model is cloned, no extra draw call or HUD substitute is used.\n'
readme+='User confirmed prior native-body, movement, and this tuned camera framing.\n'\nreadme+='New factory preset has not itself been game-regression-tested.\n'
readme+='Possible head/hair clipping, arms/weapons overlap and animation anomalies.\n'
readme+='Back up plugin DLL and hud.json, exit game, extract to game directory.\n'
readme+='Switch off to restore original logic; to roll back restore saved plugin.\n'
readme+='See docs/research/first-person-body-native-gate.md in the source ZIP.\n'
readme+='New: first-person clip plane at adjustable model-relative height; optional\n'
readme+='three-axis display-only body translation via Studio bone copies.\n'
readme+='Main menu [3] toggles body; menu [9] opens advanced cut/position pages.\n'
readme+='Factory defaults now match the supplied user-tested hud.json body preset.\n'
readme+='Evo-inspired render-only head->neck and upper-arm->clavicle branch collapse.\n'
readme+='Main menu now has ONE first-person body submenu with enable/hide settings.\n'
readme+='Head AND third-person arm hiding now default ON with body rendering.\n'
readme+='Existing body position offsets remain. Legacy GPU clip is optional.\n'
readme+='This is independent algorithm based on Evo behavior descriptions, not Evo code.\n'
readme+='New bone hiding has NOT been verified in game; MODs may not be supported.\n'
readme+='NEW dynamic anchor: model inverse-bind/eye calibration, chest/neck fallback.\n'
readme+='Native current-draw camera sampling, jump/landing vertical stabilization,\n'
readme+='optional bounded yaw correction, persistent per-MOD XYZ/yaw profiles.\n'
readme+='Dynamic anchor and camera-yaw following default ON in this preset.\n'
readme+='Save to current MOD, then save hud.json. Back up existing hud.json.\n'
readme+='This is an independent approximation of the closed-source Evo behavior,\n'
readme+='NOT an exact copy or full VR/IK reproduction. GAME EFFECT UNTESTED.\n'
readme+='The body was confirmed visible by user before these effects. New clipping\n'
readme+='and offset handling remain game-untested, including shader/deferred passes.\n'
readme+='NOT A STABLE RELEASE. Do not assume VAC safety.\n'
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
    assert config['firstPersonBody']['enabled'] is True\n    assert config['firstPersonBody']['dynamicAnchor'] is True\n    assert config['firstPersonBody']['followCameraYaw'] is True\n    assert config['firstPersonBody']['forward'] == -13\n    assert config['firstPersonBody']['hideHead'] and config['firstPersonBody']['hideArms']\n    assert config['firstPersonBody']['clipEnabled'] is False
print(build/filename)
print(build/source_name)
