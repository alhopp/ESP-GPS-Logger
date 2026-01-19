import math
import os
import requests

# ---------------- CONFIG ----------------
LAT =    -32.01372961   # latitude
LON =     115.8161796   # longitude
ZOOMS = [14, 15]      # zoom levels
RADIUS_TILES = 1     # tiles in each direction from center
OUT_DIR = "tiles"     # output folder

ESRI_URL = (
    "https://services.arcgisonline.com/ArcGIS/rest/services/"
    "World_Imagery/MapServer/tile/{z}/{y}/{x}"
)

HEADERS = {
    "User-Agent": "Mozilla/5.0"
}

# ---------------- TILE MATH ----------------
def latlon_to_tile(lat, lon, z):
    lat_rad = math.radians(lat)
    n = 2 ** z
    x = int((lon + 180.0) / 360.0 * n)
    y = int(
        (1.0 - math.log(math.tan(lat_rad) + 1 / math.cos(lat_rad)) / math.pi)
        / 2.0 * n
    )
    return x, y

# ---------------- DOWNLOAD ----------------
def download_tile(z, x, y):
    url = ESRI_URL.format(z=z, x=x, y=y)
    path = os.path.join(OUT_DIR, str(z), str(x))
    os.makedirs(path, exist_ok=True)

    fname = os.path.join(path, f"{y}.jpg")
    if os.path.exists(fname):
        return

    r = requests.get(url, headers=HEADERS, timeout=10)
    if r.status_code == 200:
        with open(fname, "wb") as f:
            f.write(r.content)
        print(f"✓ z{z} x{x} y{y}")
    else:
        print(f"✗ z{z} x{x} y{y} ({r.status_code})")

# ---------------- MAIN ----------------
for z in ZOOMS:
    cx, cy = latlon_to_tile(LAT, LON, z)

    for dx in range(-RADIUS_TILES, RADIUS_TILES + 1):
        for dy in range(-RADIUS_TILES, RADIUS_TILES + 1):
            download_tile(z, cx + dx, cy + dy)

print("Done.")
