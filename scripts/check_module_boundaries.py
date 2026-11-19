"""Keep the application/render infrastructure independent of concrete features."""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[1]
for name in ('module_host.hpp', 'hud_runtime.hpp', 'plugin.cpp', 'source_render.hpp'):
    text = (root / 'src' / name).read_text()
    for feature in ('tank::', 'rockui::', 'rockruntime::', 'deform::', 'animationfeature::'):
        assert feature not in text, f'{name} directly depends on {feature}'
    includes = re.findall(r'#include\s+"([^"]+)"', text)
    assert not any(Path(p).name.startswith(('tank_', 'rock_', 'entity_transform_')) or p.startswith('modules/') for p in includes), name
for path in (root / 'src/modules').glob('*_module.hpp'):
    includes = re.findall(r'#include\s+"([^"]+)"', path.read_text())
    assert not any(p.endswith('_module.hpp') for p in includes), f'{path.name} imports another feature adapter'
print('PASS: infrastructure has no concrete feature dependencies; feature adapters do not import each other')

for path in (root / 'src').glob('rock_*.hpp'):
    assert not re.search(r'#include\s+"tank_', path.read_text()), f'{path.name} imports Tank implementation'
for pattern in ('tank_*.hpp', 'entity_transform_*.hpp'):
    for path in (root / 'src').glob(pattern):
        assert '#include "entity_menu.hpp"' not in path.read_text(), f'{path.name} imports animation entity menu'

# Studio host remains implementation-neutral: no direct first-person dependency.
assert 'first_person_body' not in (root / 'src/source_render.hpp').read_text()
assert 'first_person_body' not in (root / 'src/plugin.cpp').read_text()
