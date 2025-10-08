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


def main():
    TEXT = "Sp3ctra_"

    # ---------------------------------------------
    # 🔧 Global scaling factor for all dimensions
    # ---------------------------------------------
    SCALE_FACTOR = 2  # Increase or decrease to globally scale the output

    # --- Derived parameters ---
    W, H = 250 * SCALE_FACTOR, 64 * SCALE_FACTOR
    FONT_PATH = "Inter/static/Inter_28pt-Bold.ttf"
    FONT_SIZEPT = 56 * SCALE_FACTOR
    PNG_OUT = f"Sp3ctra_{W}x{H}.png"

    font = load_font_or_die(FONT_PATH, FONT_SIZEPT)

    # --- base monochrome image (1 bpp, black background) ---
    img = Image.new("1", (W, H), 0)
    draw = ImageDraw.Draw(img)

    # --- measure bbox for pixel-perfect centering ---
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

    if len(TEXT) == 0:
        rgba = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        rgba.save(PNG_OUT)
        print(f"[OK] Transparent PNG saved to: {PNG_OUT}")
        return

    base_text = TEXT[:-1]
    last_char = TEXT[-1]

    draw.text((x, y), base_text, font=font, fill=1)
    base_bbox = draw.textbbox((x, y), base_text, font=font)
    base_advance = base_bbox[2] - x

    ascent, descent = font.getmetrics()

    # -------------------------------------------------
    # 🔧 Underscore adjustment (scales with SIZE too)
    # -------------------------------------------------
    EXTRA_TWEAK = -7 * SCALE_FACTOR - 1
    lift_up = descent + EXTRA_TWEAK

    last_x = x + base_advance
    last_y = y - lift_up
    draw.text((last_x, last_y), last_char, font=font, fill=1)

    # --- convert to RGBA with transparent background ---
    rgba = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    rgba.paste((255, 255, 255, 255), mask=img)
    rgba.save(PNG_OUT)

    print(f"[OK] Transparent PNG saved to: {PNG_OUT}")


if __name__ == "__main__":
    main()
