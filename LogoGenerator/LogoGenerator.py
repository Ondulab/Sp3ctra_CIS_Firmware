#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import numpy as np
import sys


def load_font_or_die(font_rel_path: str, size_pt: int):
    script_dir = Path(__file__).resolve().parent
    font_path = (script_dir / font_rel_path).resolve()

    if not font_path.exists():
        print(f"[ERROR] Font not found at: {font_path}", file=sys.stderr)
        sys.exit(1)

    try:
        return ImageFont.truetype(str(font_path), size_pt)
    except Exception as e:
        print(f"[ERROR] Failed to load font: {e}", file=sys.stderr)
        sys.exit(1)


def pack_pages_bit0_top(pixels_01: np.ndarray) -> np.ndarray:
    h, w = pixels_01.shape
    if h % 8 != 0:
        pad = np.zeros((8 - (h % 8), w), dtype=np.uint8)
        pixels_01 = np.vstack([pixels_01, pad])
        h = pixels_01.shape[0]

    pages = []
    for p in range(0, h, 8):
        block = pixels_01[p:p+8, :]
        byte_vals = (block * (1 << np.arange(8)[:, None])).sum(axis=0)
        pages.append(byte_vals.astype(np.uint8))
    return np.concatenate(pages, axis=0)


def to_c_array(name: str, arr: np.ndarray, width: int, height: int, per_line: int = 16) -> str:
    hexbytes = [f"0X{b:02X}" for b in arr]
    lines = []
    for i in range(0, len(hexbytes), per_line):
        lines.append("\t" + ",".join(hexbytes[i:i+per_line]) + ",")
    header = []
    header.append(f"// w = {width} h = {height}")
    header.append(f"const unsigned char {name}[{len(arr)}] = {{")
    header.extend(lines)
    header.append("};")
    return "\n".join(header)


def to_ascii_art(pixels: np.ndarray, on_char: str = "@", off_char: str = " "):
    h, w = pixels.shape
    lines = []
    for y in range(h):
        row = "".join(on_char if pixels[y, x] else off_char for x in range(w))
        lines.append(row)
    return lines


def downsample(pixels: np.ndarray, scale: int = 2) -> np.ndarray:
    """Downsample the bitmap by factor `scale`."""
    return pixels[::scale, ::scale]


def main():
    TEXT        = "Sp3ctra_"
    W, H        = 250, 64
    FONT_PATH   = "Inter/static/Inter_28pt-Medium.ttf"   # police Inter Medium
    FONT_SIZEPT = 50
    PNG_OUT     = "sp3ctra_inter_preview.png"
    H_OUT       = "sp3ctra_inter.h"
    SYMBOL_NAME = "Sp3ctra_img"
    ASCII_SCALE = 4   # 1=taille réelle, 2=moitié, 4=quart

    font = load_font_or_die(FONT_PATH, FONT_SIZEPT)

    img = Image.new("1", (W, H), 0)
    draw = ImageDraw.Draw(img)

    # --- centrage pixel-perfect ---
    temp = Image.new("1", (W, H), 0)
    draw_temp = ImageDraw.Draw(temp)
    draw_temp.text((0, 0), TEXT, font=font, fill=1)

    bbox = temp.getbbox()
    if bbox:
        tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
        x = (W - tw) // 2 - bbox[0]
        y = (H - th) // 2 - bbox[1]
    else:
        x, y = W // 2, H // 2

    draw.text((x, y), TEXT, font=font, fill=1)

    img.save(PNG_OUT)

    pixels = np.array(img, dtype=np.uint8)
    data   = pack_pages_bit0_top(pixels)

    expected_size = W * ((H + 7) // 8)
    assert data.size == expected_size

    # tableau C binaire
    c_code = to_c_array(SYMBOL_NAME, data, W, H, per_line=16)

    # ASCII réduit
    small_pixels = downsample(pixels, ASCII_SCALE)
    ascii_lines = to_ascii_art(small_pixels, "@", " ")
    ascii_c = [f'printf("{line}\\n");' for line in ascii_lines]

    guard = "__SP3CTRA_PICTURES_H__"
    header_text = (
        "/** Auto-generated bitmap for 'Sp3ctra_' with Inter Medium, 250x64, 1bpp.\n"
        " *  Packing: 8-pixel vertical pages, bit0 = top.\n"
        " *  Pixel-perfect centering.\n"
        " *  Includes ASCII-art printf for console preview.\n"
        f" *  ASCII_SCALE = {ASCII_SCALE}\n"
        " */\n"
        f"#ifndef {guard}\n#define {guard}\n\n"
        + c_code + "\n\n"
        "// ASCII preview:\n"
        "static inline void Sp3ctra_ascii_preview(void)\n{\n"
        + "\n".join("\t" + l for l in ascii_c) + "\n}\n\n"
        f"#endif // {guard}\n"
    )

    Path(H_OUT).write_text(header_text, encoding="utf-8")

    print(f"[OK] Preview saved to: {PNG_OUT}")
    print(f"[OK] Header saved to : {H_OUT}")


if __name__ == "__main__":
    main()