#!/usr/bin/env python3
"""Convert privately owned original music/SFX files to GBA-ready signed PCM.

The public repository DOES NOT contain the original soundtrack, nor does it
currently play these banks on real hardware. This script prepares mono s8 PCM
and word-aligned stream chunks + a manifest for later DMA/timer integration.
No generated recordings are uploaded to public GitHub.

Example:
  python3 tools/prepare_gba_audio.py /private/audio/mu_title.ogg \
      /private/audio/mu_swolvy.ogg --out /private/gba_audio
Requires ffmpeg for decoding audio inputs.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

DEFAULT_RATE = 16384
DEFAULT_CHUNK_SAMPLES = 4096


def stream_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def make_chunk_plan(total_samples: int, chunk_samples: int = DEFAULT_CHUNK_SAMPLES):
    """Plan 4-byte aligned ROM chunks without losing sample count at the tail."""
    if total_samples < 0 or chunk_samples <= 0 or chunk_samples % 4:
        raise ValueError("Need nonnegative samples and positive 4-byte chunk size")
    entries = []
    for start in range(0, total_samples, chunk_samples):
        count = min(chunk_samples, total_samples - start)
        padding = (-count) % 4
        entries.append({
            "offset_samples": start,
            "samples": count,
            "padding_bytes": padding,
            "rom_bytes": count + padding
        })
    return entries


def asset_name(path: Path):
    text = re.sub(r"[^A-Za-z0-9_]+", "_", path.stem).strip("_").lower()
    if not text or text[0].isdigit():
        text = "audio_" + text
    return text


def decode_audio(source: Path, target: Path, rate: int):
    if not source.is_file():
        raise FileNotFoundError(source)
    target.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([
        "ffmpeg", "-nostdin", "-hide_banner", "-loglevel", "error",
        "-y", "-i", str(source), "-vn", "-ac", "1", "-ar", str(rate),
        "-acodec", "pcm_s8", "-f", "s8", str(target)
    ], check=True)
    if target.stat().st_size == 0:
        raise ValueError(f"No samples decoded from {source}")


def prepare_audio(input_files: list[Path], output: Path, rate: int,
                  chunk_samples: int):
    if rate < 8192 or rate > 32768:
        raise ValueError("GBA target PCM rate must be between 8192 and 32768 Hz")
    if not input_files:
        raise ValueError("No source audio files")
    names = [asset_name(p) for p in input_files]
    if len(set(names)) != len(names):
        raise ValueError("Two source names would overwrite the same output")
    make_chunk_plan(1, chunk_samples)
    output.mkdir(parents=True, exist_ok=True)
    tracks = []
    for source, name in zip(input_files, names):
        target = output / "pcm" / f"{name}.pcm"
        decode_audio(source, target, rate)
        samples = target.stat().st_size
        plan = make_chunk_plan(samples, chunk_samples)
        bank = output / "chunks" / name
        bank.mkdir(parents=True, exist_ok=True)
        with target.open("rb") as handle:
            for idx, part in enumerate(plan):
                buf = handle.read(part["samples"])
                if len(buf) != part["samples"]:
                    raise OSError(f"Short PCM read for {name}")
                chunk = bank / f"{idx:04d}.bin"
                chunk.write_bytes(buf + b"\0" * part["padding_bytes"])
                part["relative_path"] = str(chunk.relative_to(output))
                part["sha256"] = stream_sha256(chunk)
        tracks.append({
            "name": name, "source_name": source.name,
            "source_sha256": stream_sha256(source),
            "pcm_sha256": stream_sha256(target),
            "format": "signed 8-bit mono raw PCM",
            "pcm_path": str(target.relative_to(output)),
            "sample_rate_hz": rate,
            "total_samples": samples,
            "duration_seconds": round(samples / rate, 6),
            "chunk_samples": chunk_samples,
            "chunks": plan
        })
    manifest = {
        "kind": "PRIVATE_GBA_PCM_PREPARATION_NOT_A_PLAYBACK_ENGINE",
        "tool": "ffmpeg",
        "runtime_audio_player_verified": False,
        "soundtrack_completeness_verified": False,
        "tracks": tracks
    }
    (output / "audio_manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sources", nargs="+", type=Path)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--sample-rate", type=int, default=DEFAULT_RATE)
    parser.add_argument("--chunk-samples", type=int, default=DEFAULT_CHUNK_SAMPLES)
    args = parser.parse_args()
    manifest = prepare_audio(args.sources, args.out, args.sample_rate,
                             args.chunk_samples)
    print(f"Prepared {len(manifest['tracks'])} private audio sources; "
          "GBA runtime playback is still unimplemented.")


if __name__ == "__main__":
    main()
