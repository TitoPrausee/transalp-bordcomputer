#!/usr/bin/env python3
"""Export SD-Card CSV logs to GPX track files.

Usage:
    python3 export_track.py /sdcard/logs/SESSION_20261013_140000.csv -o track.gpx
"""

import csv
import sys
import argparse
from datetime import datetime, timezone


def csv_to_gpx(csv_path, gpx_path):
    points = []
    with open(csv_path, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                lat = float(row.get("lat", 0))
                lon = float(row.get("lon", 0))
                ele = float(row.get("alt", 0))
                ts = float(row.get("timestamp", 0))
            except (ValueError, TypeError):
                continue
            if lat == 0 and lon == 0:
                continue
            points.append((lat, lon, ele, ts))

    if not points:
        print(f"No valid GPS points in {csv_path}")
        return

    with open(gpx_path, "w") as f:
        f.write('<?xml version="1.0" encoding="UTF-8"?>\n')
        f.write('<gpx version="1.1" creator="transalp-bordcomputer" '
                'xmlns="http://www.topografix.com/GPX/1/1">\n')
        f.write('  <trk>\n')
        f.write(f'    <name>{csv_path.stem}</name>\n')
        f.write('    <trkseg>\n')
        for lat, lon, ele, ts in points:
            dt = datetime.fromtimestamp(ts / 1000.0, tz=timezone.utc)
            tstr = dt.strftime("%Y-%m-%dT%H:%M:%SZ")
            f.write(f'      <trkpt lat="{lat:.6f}" lon="{lon:.6f}">\n')
            f.write(f'        <ele>{ele:.1f}</ele>\n')
            f.write(f'        <time>{tstr}</time>\n')
            f.write('      </trkpt>\n')
        f.write('    </trkseg>\n')
        f.write('  </trk>\n')
        f.write('</gpx>\n')

    print(f"Exported {len(points)} points → {gpx_path}")


def main():
    parser = argparse.ArgumentParser(description="Export CSV track to GPX")
    parser.add_argument("csv", help="Input CSV file")
    parser.add_argument("-o", "--output", default="track.gpx", help="Output GPX file")
    args = parser.parse_args()

    csv_to_gpx(args.csv, args.output)


if __name__ == "__main__":
    main()
