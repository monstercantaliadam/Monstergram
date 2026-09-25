from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).parent
SIZE = 1024
VIEW = 1200
LEFT = [(205, 825), (350, 330), (600, 635), (600, 845), (409, 595), (333, 825)]
RIGHT = [(VIEW - x, y) for x, y in LEFT]
DART = [(600, 245), (662, 515), (600, 462), (538, 515)]
PALETTES = {
    "monstergram-midnight": ("#171C36", "#33305D", "#F8F8FF", "#D5C7FF", "#9B79FF"),
    "monstergram-ocean": ("#103B67", "#18799A", "#F5FCFF", "#B8F3FF", "#52D7F5"),
    "monstergram-sunset": ("#74304E", "#DC6859", "#FFF9F3", "#FFE2CF", "#FFBA83"),
    "monstergram-forest": ("#143F3F", "#28846C", "#F4FFF9", "#B8F3D9", "#6EE6AF"),
}


def rgb(value):
    return tuple(bytes.fromhex(value[1:]))


def scaled(points):
    return [(round(x * SIZE / VIEW), round(y * SIZE / VIEW)) for x, y in points]


def path(points):
    return "M" + " L".join(f"{x} {y}" for x, y in points) + " Z"


def make_svg(name, colors):
    top, bottom, left, right, accent = colors
    body = f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1200 1200" width="1200" height="1200">
<defs>
  <linearGradient id="background" x1="0%" y1="0%" x2="100%" y2="100%">
    <stop offset="0%" stop-color="{top}"/>
    <stop offset="100%" stop-color="{bottom}"/>
  </linearGradient>
  <clipPath id="circle"><circle cx="600" cy="600" r="600"/></clipPath>
</defs>
<g clip-path="url(#circle)">
  <circle cx="600" cy="600" r="600" fill="url(#background)"/>
  <circle cx="240" cy="130" r="350" fill="{accent}" opacity="0.12"/>
  <path d="{path(LEFT)}" fill="{left}"/>
  <path d="{path(RIGHT)}" fill="{right}"/>
  <path d="{path(DART)}" fill="{accent}"/>
</g>
</svg>
'''
    folder = ROOT / name
    folder.mkdir(exist_ok=True)
    with (folder / "app.svg").open("w", encoding="utf-8", newline="\r\n") as output:
        output.write(body)


def make_icon(name, colors):
    top, bottom, left, right, accent = map(rgb, colors)
    image = Image.new("RGBA", (SIZE, SIZE))
    pixels = image.load()
    for y in range(SIZE):
        for x in range(SIZE):
            progress = (x + y) / (2 * (SIZE - 1))
            color = tuple(round(a + (b - a) * progress) for a, b in zip(top, bottom))
            pixels[x, y] = (*color, 255)

    orb = Image.new("RGBA", (SIZE, SIZE))
    draw = ImageDraw.Draw(orb)
    draw.ellipse(
        (round(-110 * SIZE / VIEW), round(-220 * SIZE / VIEW),
         round(590 * SIZE / VIEW), round(480 * SIZE / VIEW)),
        fill=(*accent, 31),
    )
    image = Image.alpha_composite(image, orb)
    draw = ImageDraw.Draw(image)
    draw.polygon(scaled(LEFT), fill=left)
    draw.polygon(scaled(RIGHT), fill=right)
    draw.polygon(scaled(DART), fill=accent)

    mask = Image.new("L", (SIZE, SIZE))
    ImageDraw.Draw(mask).ellipse((0, 0, SIZE - 1, SIZE - 1), fill=255)
    image.putalpha(mask)
    image.save(
        ROOT / name / "app_icon.ico",
        format="ICO",
        sizes=[(size, size) for size in (16, 24, 32, 48, 64, 128, 256)],
    )


for icon_name, palette in PALETTES.items():
    make_svg(icon_name, palette)
    make_icon(icon_name, palette)
