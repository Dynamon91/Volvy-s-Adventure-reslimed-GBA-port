#!/usr/bin/env python3
"""Test GBA audio stream indexing without needing private music or ffmpeg."""
import json
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import prepare_gba_audio as gba

assert gba.make_chunk_plan(0, 4096) == []
assert gba.make_chunk_plan(1, 4096) == [{
    "offset_samples": 0, "samples": 1, "padding_bytes": 3,
    "rom_bytes": 4
}]
assert gba.make_chunk_plan(4096, 4096)[0]["padding_bytes"] == 0
test = gba.make_chunk_plan(4097, 4096)
assert len(test) == 2
assert test[0]["samples"] == 4096
assert test[1]["samples"] == 1
assert test[1]["offset_samples"] == 4096
assert test[1]["padding_bytes"] == 3
assert sum(x["samples"] for x in test) == 4097
assert all(x["rom_bytes"] % 4 == 0 for x in test)
try:
    gba.make_chunk_plan(100, 1023)
except ValueError:
    pass
else:
    raise AssertionError("Unaligned stream chunk accepted")

assert gba.asset_name(Path("MU_TITLE.ogg")) == "mu_title"
assert gba.asset_name(Path("weird name!.ogg")) == "weird_name"
with tempfile.TemporaryDirectory() as temp:
    folder = Path(temp)
    source = folder / "mu_title.ogg"
    source.write_bytes(b"public synthetic audio test only")
    original_decoder = gba.decode_audio

    def synthetic_decode(source_file, target, rate):
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(bytes([0, 255, 127, 128]) * 1025 + b"x")
    gba.decode_audio = synthetic_decode
    try:
        output = folder / "output"
        manifest = gba.prepare_audio([source], output, 16384, 4096)
    finally:
        gba.decode_audio = original_decoder
    track = manifest["tracks"][0]
    assert track["sample_rate_hz"] == 16384
    assert track["total_samples"] == 4101
    assert track["format"] == "signed 8-bit mono raw PCM"
    assert len(track["chunks"]) == 2
    assert track["chunks"][0]["samples"] == 4096
    assert track["chunks"][1]["samples"] == 5
    assert track["chunks"][1]["padding_bytes"] == 3
    assert track["chunks"][1]["rom_bytes"] == 8
    assert (output / track["chunks"][1]["relative_path"]).stat().st_size == 8
    assert manifest["runtime_audio_player_verified"] is False
    assert manifest["soundtrack_completeness_verified"] is False
    assert json.loads((output / "audio_manifest.json").read_text()) == manifest

print("GBA audio bank pipeline: PASS (sample math, 4-byte chunks, exact last sample and private manifest)")
