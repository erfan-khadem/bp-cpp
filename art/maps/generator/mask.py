import sys
import random

from PIL import Image, ImageDraw

DIM_X = 1280
DIM_Y = 800

RAD_BIG = 40
RADS = [
    10,
    15,
    5,
    8,
    17
]

NUM_RANDS = 60

FILLS = [
    "#865e3c",
    "#986a44",
    "#b5835a",
    "#63452c",
]

def draw_circle(draw: ImageDraw, radius: int, center: tuple[int, int], fill: str | tuple) -> None:
    draw.ellipse(
            (center[0] - radius, center[1] - radius,
             center[0] + radius, center[1] + radius), fill=fill)


def main() -> None:
    center_of_path = []
    file_name = sys.argv[1]
    with open(file_name + ".txt", "r") as f:
        data = f.read().split()
        data = data[2:]
        data = map(int, data)
        prev = 0
        for i, v in enumerate(data):
            if i & 1:
                center_of_path.append((prev, v))
            else:
                prev = v

    # center of path -> (x, y)
    image = Image.new('RGBA', (DIM_X, DIM_Y))
    draw = ImageDraw.Draw(image)

    for x, y in center_of_path:
        draw_circle(draw, RAD_BIG, (x, y), FILLS[0])

    rands = []
    for i in range(len(center_of_path) // NUM_RANDS):
        rands.append(random.choice(center_of_path[i * NUM_RANDS:(i+1) * NUM_RANDS]))

    for x, y in rands:
        x += random.randint(-RAD_BIG + 20, RAD_BIG - 20)
        y += random.randint(-RAD_BIG + 20, RAD_BIG - 20)
        rad = random.choice(RADS)
        color = random.choice(FILLS[1:])
        draw_circle(draw, rad, (x, y), color)

    image.save(f"{file_name}.png")


if __name__ == "__main__":
    random.seed(228)
    main()
