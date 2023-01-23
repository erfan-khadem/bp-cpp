from PIL import Image, ImageDraw, ImageFilter


DIM_X = 1280
DIM_Y = 1280

FILLS = [
    ("#1c71d8", "#62a0ea"),
    ("#e66100", "#ffa348"),
    ("#f5c211", "#f8e45c"),
    ("#c01c28", "#ed333b"),
    ("#813d9c", "#c061cb")
]

def draw_circle(draw: ImageDraw, radius: int, fill: str | tuple) -> None:
    draw.ellipse((radius, radius, DIM_X - radius, DIM_Y - radius), fill=fill)


def main() -> None:

    for i, (a, b) in enumerate(FILLS):
        image = Image.new('RGBA', (DIM_X, DIM_Y))
        draw = ImageDraw.Draw(image)

        draw_circle(draw, 128, a)
        draw_circle(draw, 220, b)
        draw_circle(draw, 280, a)

        image_final = image.filter(ImageFilter.GaussianBlur(10))

        image_final.save("ball{:02}.png".format(i + 1))


if __name__ == "__main__":
    main()
