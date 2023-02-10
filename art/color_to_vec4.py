inp = input()[1:]

ln = len(inp)

inp = int(inp, 16)
if ln == 8:
    alpha = inp & 0xff
    inp >>= 8
else:
    alpha = 0xff

r = (inp & 0xff0000) >> 16
g = (inp & 0x00ff00) >> 8
b = (inp & 0x0000ff) >> 0

rd_div = lambda x : round(x / 255, 2)

r = rd_div(r)
g = rd_div(g)
b = rd_div(b)
alpha = rd_div(alpha)

print(f"ImVec4({r}, {g}, {b}, {alpha})")
