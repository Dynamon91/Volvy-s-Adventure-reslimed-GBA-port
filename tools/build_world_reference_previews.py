#!/usr/bin/env python3
"""Generate private, source-grounded GBA Mode 4 reference previews for all indexed worlds.

This DOES NOT create playable levels, source tile maps or collision.
The original six-part walkthrough must be joined outside the public repo.
The output is a contact sheet, a GIF, source/frame SHA256 manifest and one
240x160 palette-faithful PNG per checkpoint. Original artwork stays private.

Example:
    python3 tools/build_world_reference_previews.py \
        /private/walkthrough_joined.mp4 --out /private/volvy_world_previews
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import zipfile

from PIL import Image, ImageDraw, ImageFont

W, H, IMAGE_H, TOP = 240, 160, 135, 12


def color_to_bgr555(rgb):
    r, g, b = rgb
    return ((r * 31 + 127) // 255) | (((g * 31 + 127) // 255) << 5) | (
        ((b * 31 + 127) // 255) << 10
    )


def bgr555_preview_rgb(word):
    expand = lambda n: (n << 3) | (n >> 2)
    return (expand(word & 31), expand((word >> 5) & 31),
            expand((word >> 10) & 31))


def make_mode4_preview(source: Image.Image) -> Image.Image:
    """Preserve 16:9 video content at 240x135, black 12/13px letterbox.

    Reserve index 0 for the black margin and quantize the visible frame into
    a maximum of 255 other colors. Return a P-mode indexed PNG whose palette
    matches exactly what 8bpp Mode 4's GBA BGR555 values would display.
    """
    source = source.convert("RGB")
    if source.width * 9 != source.height * 16:
        raise ValueError(f"Expected 16:9 video frame: got {source.size}")
    content = source.resize((W, IMAGE_H), Image.Resampling.BOX)
    quantized = content.quantize(
        colors=255, method=Image.Quantize.MEDIANCUT,
        dither=Image.Dither.NONE
    )
    rgb8 = quantized.getpalette() or []
    if len(rgb8) < 255 * 3:
        rgb8 += [0] * (255 * 3 - len(rgb8))
    colors = [(0, 0, 0)] + [
        tuple(rgb8[i:i + 3]) for i in range(0, 255 * 3, 3)
    ]
    device_colors = [bgr555_preview_rgb(color_to_bgr555(c)) for c in colors]
    frame = Image.new("P", (W, H), 0)
    frame_indices = bytes(value + 1 for value in quantized.tobytes())
    frame.paste(Image.frombytes("P", (W, IMAGE_H), frame_indices), (0, TOP))
    palette = [component for rgb in device_colors for component in rgb]
    frame.putpalette(palette)
    return frame


def file_sha256(path: Path):
    """Hash the entire private recording without allocating a 600 MB buffer."""
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_index(path: Path):
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("schema_version") != 1:
        raise ValueError("Unsupported reference index schema")
    scenes = data.get("checkpoints") or []
    if not scenes:
        raise ValueError("Empty original-video reference index")
    ids = [scene["id"] for scene in scenes]
    times = [scene["second"] for scene in scenes]
    if len(ids) != len(set(ids)) or sorted(times) != times or len(times) != len(set(times)):
        raise ValueError("Scene IDs and source offsets must be unique and sorted")
    if any(type(t) is not int or t < 0 or t >= data["reference_video_duration_seconds"] for t in times):
        raise ValueError("Source frame timestamp outside recorded video")
    for scene in scenes:
        if scene["course_in_native_gba"] is not False:
            raise ValueError("Source-only screenshot cannot prove native gameplay")
        if not scene.get("label") or not scene.get("confirmation"):
            raise ValueError("Missing source evidence")
    return data


def extract_frame(video: Path, seconds: int, out_path: Path):
    out_path.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([
        "ffmpeg", "-hide_banner", "-loglevel", "error",
        "-ss", str(seconds), "-i", str(video),
        "-frames:v", "1", "-y", str(out_path)
    ], check=True)
    if not out_path.is_file():
        raise RuntimeError(f"Could not extract video frame at {seconds}s")


def make_sheet(items, images: list[Image.Image], output: Path):
    card_w, card_h, columns = 496, 382, 3
    rows = (len(items) + columns - 1) // columns
    sheet = Image.new("RGB", (columns * card_w, 94 + rows * card_h),
                      (17, 22, 34))
    pen = ImageDraw.Draw(sheet)
    font = ImageFont.load_default()
    pen.text((22, 18), "VOLVY GBA / ORIGINAL-WORLD REFERENCE FRAMES",
             font=font, fill="white")
    pen.text((22, 46),
             "MODE 4 PALETTE SIMULATION | SOURCE VIDEO | NOT EMULATOR IMAGES",
             font=font, fill=(180, 192, 215))
    for i, (scene, img) in enumerate(zip(items, images)):
        x = 12 + (i % columns) * card_w
        y = 96 + (i // columns) * card_h
        thumbnail = img.convert("RGB").resize((480, 320),
                                              Image.Resampling.NEAREST)
        sheet.paste(thumbnail, (x, y + 26))
        label = f"{i + 1:02d} {scene['label']}  {scene['second'] // 60}:{scene['second'] % 60:02d}"
        pen.text((x + 2, y + 5), label, font=font, fill="white")
        pen.text((x + 2, y + 349), "SOURCE-BASED PREVIEW / NOT A PLAYABLE SCENE",
                 font=font, fill=(183, 197, 219))
    sheet.save(output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("video", type=Path)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument(
        "--index", type=Path,
        default=Path(__file__).resolve().parents[1] /
                "progress/reference_scene_checkpoints.json"
    )
    args = parser.parse_args()
    if not args.video.is_file():
        parser.error("Original user-supplied recording not found")
    data = load_index(args.index)
    out = args.out.resolve()
    (out / "mode4_previews").mkdir(parents=True, exist_ok=True)
    source_tmp = out / "source_frames"
    source_tmp.mkdir(exist_ok=True)
    images = []
    metadata = []
    for scene in data["checkpoints"]:
        source_path = source_tmp / (scene["id"] + ".png")
        target_path = out / "mode4_previews" / (scene["id"] + ".png")
        extract_frame(args.video, scene["second"], source_path)
        original = Image.open(source_path)
        converted = make_mode4_preview(original)
        converted.save(target_path)
        images.append(converted)
        metadata.append({
            "id": scene["id"], "source_time": scene["second"],
            "source_sha256": hashlib.sha256(source_path.read_bytes()).hexdigest(),
            "preview_sha256": hashlib.sha256(target_path.read_bytes()).hexdigest(),
            "mode": "indexed 8bpp, BGR555 re-expanded",
            "frame": [W, H], "letterbox": [12, 13],
            "emulator_capture": False, "gameplay_scene_implemented": False
        })
    sheet = out / "all_worlds_gba_reference_sheet.png"
    make_sheet(data["checkpoints"], images, sheet)

    # The GIF encodes these visual checkpoint images; it is not a sprite GIF.
    frames = []
    font = ImageFont.load_default()
    for scene, src in zip(data["checkpoints"], images):
        gif_frame = Image.new("RGB", (480, 354), (17, 22, 34))
        gif_frame.paste(src.convert("RGB").resize(
            (480, 320), Image.Resampling.NEAREST), (0, 24))
        ImageDraw.Draw(gif_frame).text(
            (8, 7), f"{scene['label']} / VIDEO {scene['second']}s",
            font=font, fill="white")
        frames.append(gif_frame)
    frames[0].save(out / "all_worlds_gba_reference.gif",
                   save_all=True, append_images=frames[1:],
                   duration=950, loop=0, disposal=2)

    manifest = {
        "kind": "PRIVATE_SOURCE_VIDEO_TO_GBA_REFERENCE_PREVIEW_ONLY",
        "video_sha256": file_sha256(args.video),
        "frame_count": len(images),
        "not_emulator_verified": True,
        "no_gameplay_implementation_claimed": True,
        "previews": metadata
    }
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    bundle = out / "world_reference_previews.zip"
    with zipfile.ZipFile(bundle, "w", compression=zipfile.ZIP_DEFLATED) as z:
        for path in [sheet, out / "all_worlds_gba_reference.gif",
                     out / "manifest.json", *sorted((out / "mode4_previews").glob("*.png"))]:
            z.write(path, path.relative_to(out))
    print(f"Prepared {len(images)} private checkpoints: {bundle}")


if __name__ == "__main__":
    main()
