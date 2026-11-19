"""Verify the experimental Windows x86 package and module source boundaries."""
import json
from pathlib import Path
import struct
import zipfile

build = Path("build")
install = build / "L4D2Visual-ModuleBody-Install.zip"
source = build / "L4D2Visual-ModuleBody-Source.zip"
with zipfile.ZipFile(install) as archive:
    assert archive.testzip() is None
    names = set(archive.namelist())
    assert {p.split("/")[0] for p in names} == {"bin", "README.md"}
    dll = archive.read("bin/neko/plugins/L4D2Visual.dll")
    offset = struct.unpack_from("<I", dll, 0x3C)[0]
    assert dll[offset:offset + 4] == b"PE\x00\x00"
    assert struct.unpack_from("<H", dll, offset + 4)[0] == 0x14C
    assert b"0.8.0-module-body-experimental" in dll
    config = json.loads(archive.read("bin/neko/plugins/L4D2Visual/hud.json"))
    assert config["logging"]["level"] == "info"
    body = config["firstPersonBody"]
    assert body["enabled"] and body["dynamicAnchor"] and body["followCameraYaw"]
    assert body["hideHead"] and body["hideArms"] and not body["clipEnabled"]
    assert (body["forward"], body["right"], body["up"]) == (-13, -1, -1)
    assert "wallPictures" not in config and "sprayP2P" not in config
    assert not any("/images/" in p or "spray-cache" in p for p in names)
    readme = archive.read("README.md").decode("utf-8")
    assert "实验分支" in readme
with zipfile.ZipFile(source) as archive:
    assert archive.testzip() is None
    names = set(archive.namelist())
    required = {
        "src/module_host.hpp",
        "src/feature_composition.hpp",
        "src/modules/first_person_body_module.hpp",
        "src/first_person_body_effects.hpp",
        "src/first_person_dynamic_alignment.hpp",
        "src/log_settings.hpp",
        "tests/module_host_test.cpp",
        "tests/first_person_dynamic_alignment_test.cpp",
    }
    assert required.issubset(names), required - names
    assert not any(p.startswith("src/spray_") or p.startswith("src/image_") for p in names)
    renderer = archive.read("src/source_render.hpp").decode("utf-8")
    assert "studioDecorator" in renderer and "first_person_body" not in renderer
print("PASS: x86 DLL, installation, source, module boundaries, no sprays, original baseline")
