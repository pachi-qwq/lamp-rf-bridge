# SPDX-License-Identifier: MIT
"""Export iOS PNG icons and a dark launch image from SVG; only regeneration requires Pillow.
导出简单 SVG 的 iOS PNG 图标及深色启动图；仅重新导出需要 Pillow。
"""
from pathlib import Path
import xml.etree.ElementTree as ET
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent

def render_icon(size):
    scale = 4
    image = Image.new("RGB", (size * scale, size * scale), "#141414")
    factor = size * scale / 512
    for node in ET.parse(ROOT / "web/icon.svg").getroot():
        tag = node.tag.rsplit("}", 1)[-1]
        if tag == "rect": continue
        layer = Image.new("RGBA", image.size, (0, 0, 0, 0))
        draw = ImageDraw.Draw(layer)
        color = node.get("fill") or node.get("stroke")
        alpha = int(255 * float(node.get("opacity", "1")))
        rgb = tuple(int(color[i:i+2], 16) for i in (1, 3, 5)) + (alpha,)
        if tag == "polygon":
            points = [tuple(float(n) * factor for n in p.split(",")) for p in node.get("points").split()]
            draw.polygon(points, fill=rgb)
        elif tag == "line":
            x1, y1, x2, y2 = (float(node.get(k)) * factor for k in ("x1", "y1", "x2", "y2"))
            width = round(float(node.get("stroke-width")) * factor)
            draw.line((x1, y1, x2, y2), fill=rgb, width=width)
            radius = width / 2
            for x, y in ((x1, y1), (x2, y2)):
                draw.ellipse((x-radius, y-radius, x+radius, y+radius), fill=rgb)
        else:
            raise ValueError("图标导出器不支持的 SVG 元素：" + tag)
        image = Image.alpha_composite(image.convert("RGBA"), layer).convert("RGB")
    return image.resize((size, size), Image.Resampling.LANCZOS)

def main():
    render_icon(180).save(ROOT / "web/apple-touch-icon.png", optimize=True)
    # Use iPhone 17 Pro standard portrait pixels; the launch image reports neither connectivity nor lamp state.
    # iPhone 17 Pro 标准显示模式的竖屏像素尺寸。启动画面不代表设备在线或灯具状态。
    launch = Image.new("RGB", (1206, 2622), "#141414")
    icon = render_icon(324)
    launch.paste(icon, ((1206 - icon.width) // 2, (2622 - icon.height) // 2))
    launch.save(ROOT / "web/launch-1206x2622-v1.png", optimize=True)
    print("Exported iOS icon and dark 1206x2622 launch image; SVG remains primary")

if __name__ == "__main__":
    main()
