import sys
import math

import numpy as np
from scipy import interpolate

def main() -> None:
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} input-locations.txt output-locations.txt")
        sys.exit(1)

    new_locations = sys.argv[1]
    result_locations = sys.argv[2]

    with open(new_locations, "r") as f:
        data = f.read().split()
    arr = [[], []]
    for i, s in enumerate(data):
        arr[i&1].append(int(s))

    total_length = 0
    val = (arr[0][0], arr[0][1])
    for x, y in zip(*arr):
        total_length += math.dist(val, (x, y))
        val = (x, y)

    indx = np.linspace(0, len(arr[0]), len(arr[0]))
    ln = len(arr[0])
    fx = interpolate.interp1d(indx, arr[0], kind='cubic')
    fy = interpolate.interp1d(indx, arr[1], kind='cubic')
    cnt = int(total_length * 1.4)
    indx_res = np.linspace(0, ln, cnt * 100)
    xn = fx(indx_res)
    yn = fy(indx_res)

    ball_diameter = 64  # Pixels
    reference_dist = math.dist((xn[0], yn[0]), (xn[99], yn[99]))
    res = [(xn[0], yn[0])]
    for x, y in zip(xn, yn):
        d = math.dist((x, y), res[-1])
        if d >= reference_dist:
            res.append((x, y))

    with open(result_locations, "w") as nf:
        nf.write(f"{ball_diameter / reference_dist}\n")
        nf.write(f"{len(res)}\n")
        for x, y in res:
            nf.write(f"{round(x)} {round(y)}\n")


if __name__ == "__main__":
    main()
