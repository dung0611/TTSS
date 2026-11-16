#!/usr/bin/env python3
"""Rebuild Vietnamese benchmark charts from raw.csv and saved iteration histories."""
import argparse
import csv
import html
import json
import math
import statistics
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


W, H = 1600, 1100
LEFT, RIGHT, TOP, BOTTOM = 180, 1460, 155, 875
COLORS = {1: "#1769aa", 2: "#d1495b", 4: "#2a9d8f", 8: "#7b5ea7"}
THREADS = [1, 2, 4, 8]
FONT_DIR = Path("/usr/share/fonts/truetype/dejavu")


def font(size, bold=False):
    name = "DejaVuSans-Bold.ttf" if bold else "DejaVuSans.ttf"
    try:
        return ImageFont.truetype(str(FONT_DIR / name), size)
    except OSError:
        return ImageFont.load_default()


class Figure:
    def __init__(self):
        self.ops = []
        self.text("openmp benchmark", -500, -500, 10)
        self.rect(0, 0, W, H, "#ffffff", stroke="none")

    def line(self, points, color="#222222", width=2, dash=None):
        self.ops.append(("line", points, color, width, dash))

    def rect(self, x1, y1, x2, y2, fill, stroke="none", width=1):
        self.ops.append(("rect", x1, y1, x2, y2, fill, stroke, width))

    def circle(self, x, y, radius, fill, stroke=None, width=2):
        self.ops.append(("circle", x, y, radius, fill, stroke or fill, width))

    def text(self, value, x, y, size=24, color="#222222", align="left", bold=False, angle=0):
        self.ops.append(("text", str(value), x, y, size, color, align, bold, angle))

    def png(self, path):
        image = Image.new("RGB", (W, H), "white")
        draw = ImageDraw.Draw(image)
        for op in self.ops:
            if op[0] == "rect":
                _, x1, y1, x2, y2, fill, stroke, width = op
                draw.rectangle((x1, y1, x2, y2), fill=fill,
                               outline=None if stroke == "none" else stroke, width=width)
            elif op[0] == "line":
                _, points, color, width, dash = op
                for first, second in zip(points, points[1:]):
                    if not dash:
                        draw.line((first, second), fill=color, width=width)
                    else:
                        dx, dy = second[0] - first[0], second[1] - first[1]
                        length = math.hypot(dx, dy)
                        if not length:
                            continue
                        step = dash * 2
                        pos = 0.0
                        while pos < length:
                            end = min(pos + dash, length)
                            draw.line(((first[0] + dx * pos / length, first[1] + dy * pos / length),
                                       (first[0] + dx * end / length, first[1] + dy * end / length)),
                                      fill=color, width=width)
                            pos += step
            elif op[0] == "circle":
                _, x, y, radius, fill, stroke, width = op
                draw.ellipse((x-radius, y-radius, x+radius, y+radius), fill=fill,
                             outline=stroke, width=width)
            else:
                _, value, x, y, size, color, align, bold, angle = op
                f = font(size, bold)
                if angle:
                    layer = Image.new("RGBA", (size * max(len(value), 4), size * 2), (255,255,255,0))
                    ImageDraw.Draw(layer).text((size // 2, size // 2), value, font=f, fill=color)
                    layer = layer.rotate(angle, expand=True)
                    px, py = int(x-layer.width/2), int(y-layer.height/2)
                    image.paste(layer, (px, py), layer)
                else:
                    box = draw.textbbox((0, 0), value, font=f)
                    width_px = box[2] - box[0]
                    px = x - width_px / 2 if align == "center" else x - width_px if align == "right" else x
                    draw.text((px, y), value, font=f, fill=color)
        image.save(path, dpi=(220, 220), optimize=True)

    def svg(self, path):
        body = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">',
                '<rect width="100%" height="100%" fill="white"/>']
        for op in self.ops:
            if op[0] == "rect":
                _, x1, y1, x2, y2, fill, stroke, width = op
                body.append(f'<rect x="{x1}" y="{y1}" width="{x2-x1}" height="{y2-y1}" fill="{fill}" stroke="{stroke}" stroke-width="{width}"/>')
            elif op[0] == "line":
                _, points, color, width, dash = op
                point_text = " ".join(f"{x},{y}" for x, y in points)
                dashed = f' stroke-dasharray="{dash} {dash}"' if dash else ""
                body.append(f'<polyline points="{point_text}" fill="none" stroke="{color}" stroke-width="{width}"{dashed}/>')
            elif op[0] == "circle":
                _, x, y, radius, fill, stroke, width = op
                body.append(f'<circle cx="{x}" cy="{y}" r="{radius}" fill="{fill}" stroke="{stroke}" stroke-width="{width}"/>')
            else:
                _, value, x, y, size, color, align, bold, angle = op
                anchor = {"left":"start", "center":"middle", "right":"end"}[align]
                transform = f' transform="rotate({angle} {x} {y})"' if angle else ""
                weight = "700" if bold else "400"
                body.append(f'<text x="{x}" y="{y+size}" text-anchor="{anchor}" font-family="DejaVu Sans, sans-serif" font-size="{size}" font-weight="{weight}" fill="{color}"{transform}>{html.escape(value)}</text>')
        body.append("</svg>")
        Path(path).write_text("\n".join(body) + "\n", encoding="utf-8")

    def save(self, stem, output):
        self.png(output / f"{stem}.png")
        self.svg(output / f"{stem}.svg")


def csv_rows(path):
    with Path(path).open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def axes(title, subtitle, y_label, x_label, y_max, ticks, xlabels):
    fig = Figure()
    fig.text(title, W/2, 38, 38, bold=True, align="center")
    fig.text(subtitle, W/2, 92, 21, color="#555555", align="center")
    fig.text(y_label, LEFT, 126, 22, bold=True)
    fig.text(x_label, (LEFT+RIGHT)/2, 975, 22, bold=True, align="center")
    for tick in ticks:
        y = BOTTOM - (tick / y_max) * (BOTTOM-TOP)
        fig.line([(LEFT, y), (RIGHT, y)], "#d9dfe5", 1)
        fig.text(f"{tick:g}", LEFT-18, y-14, 18, color="#555555", align="right")
    fig.line([(LEFT, TOP), (LEFT, BOTTOM), (RIGHT, BOTTOM)], "#333333", 2)
    xs = [LEFT + (RIGHT-LEFT)*i/(len(xlabels)-1) for i in range(len(xlabels))]
    for x, label in zip(xs, xlabels):
        fig.line([(x, BOTTOM), (x, BOTTOM+8)], "#333333", 2)
        fig.text(label, x, BOTTOM+18, 18, align="center")
    return fig, xs, lambda value: BOTTOM - value/y_max*(BOTTOM-TOP)


def actual_labels(summary):
    labels = []
    for row in summary:
        requested = int(row["requested_threads"])
        actual = row["actual_threads_values"].split(";")
        labels.append(f"{requested}\n(thực tế {','.join(actual)})" if actual != [str(requested)] else str(requested))
    return labels


def draw_legend(fig, entries, y=1040):
    x = 220
    for color, label, marker in entries:
        fig.line([(x, y+12), (x+42, y+12)], color, 4)
        if marker:
            fig.circle(x+21, y+12, 7, color)
        fig.text(label, x+52, y, 18)
        x += 52 + len(label)*12 + 48


def execution_chart(raw, summary, output):
    values = [float(r[k]) for r in raw if r["run_kind"] == "measurement"
              for k in ("search_seconds", "total_seconds")]
    ymax = math.ceil(max(values)*1.12)
    ticks = [ymax*i/5 for i in range(6)]
    fig, xs, y = axes("Thời gian chạy theo số luồng",
        "Trung vị của 15 phép đo; thanh sai số là Q1–Q3 trên 3 seed và 5 lần lặp",
        "Thời gian (giây)", "Số luồng yêu cầu (số thực tế ghi dưới nhãn nếu khác)",
        ymax, ticks, actual_labels(summary))
    series = [("search_seconds", "#1769aa", "Tìm kiếm"),
              ("total_seconds", "#d1495b", "Toàn chương trình")]
    for key, color, label in series:
        pts = []
        for x, row in zip(xs, summary):
            group = [float(r[key]) for r in raw if r["run_kind"] == "measurement"
                     and int(r["requested_threads"]) == int(row["requested_threads"])]
            median = statistics.median(group)
            q1, q3 = statistics.quantiles(group, n=4, method="inclusive")[0], statistics.quantiles(group, n=4, method="inclusive")[2]
            xx = x + (-18 if key == "search_seconds" else 18)
            fig.line([(xx, y(q1)), (xx, y(q3))], color, 3)
            fig.line([(xx-8, y(q1)), (xx+8, y(q1))], color, 3)
            fig.line([(xx-8, y(q3)), (xx+8, y(q3))], color, 3)
            pts.append((xx, y(median)))
        fig.line(pts, color, 4)
        for px, py in pts:
            fig.circle(px, py, 8, color)
    draw_legend(fig, [("#1769aa", "Tìm kiếm", True), ("#d1495b", "Toàn chương trình", True)])
    fig.save("execution_time", output)


def speedup_chart(summary, output):
    max_value = max(8.5, max(float(r["paired_search_speedup_median"])
                            for r in summary)*1.15)
    max_value = math.ceil(max_value)
    ticks = list(range(0, max_value+1, max(1, math.ceil(max_value/8))))
    fig, xs, y = axes("Tốc độ tăng tốc",
        "Tỷ số ghép cùng seed; trung vị qua ba seed (OpenMP 1 luồng làm baseline)",
        "Speedup (lần)", "Số luồng yêu cầu (thực tế trong nhãn nếu khác)",
        max_value, ticks, actual_labels(summary))
    ideal = [(x, y(float(row["actual_threads_median"]))) for x, row in zip(xs, summary)]
    fig.line(ideal, "#777777", 3, 12)
    for key, color, label, offset in [
            ("paired_search_speedup_median", "#1769aa", "Tìm kiếm", -12),
            ("paired_total_speedup_median", "#d1495b", "Toàn chương trình", 12)]:
        pts = [(x+offset, y(float(row[key]))) for x, row in zip(xs, summary)]
        fig.line(pts, color, 4)
        for px, py in pts:
            fig.circle(px, py, 8, color)
    draw_legend(fig, [("#1769aa", "Tìm kiếm", True), ("#d1495b", "Toàn chương trình", True),
                      ("#777777", "Lý tưởng S=p theo số luồng thực tế", False)])
    fig.save("speedup", output)


def efficiency_chart(summary, output):
    fig, xs, y = axes("Hiệu suất tăng tốc phần tìm kiếm",
        "Search efficiency = search speedup / số luồng thực tế; đây không phải mức sử dụng CPU",
        "Hiệu suất tăng tốc (%)", "Số luồng yêu cầu (thực tế trong nhãn nếu khác)",
        110, [0,20,40,60,80,100], actual_labels(summary))
    fig.line([(LEFT, y(100)), (RIGHT, y(100))], "#777777", 3, 12)
    pts = [(x, y(float(row["paired_search_efficiency_percent"]))) for x, row in zip(xs, summary)]
    fig.line(pts, "#2a9d8f", 4)
    for px, py in pts:
        fig.circle(px, py, 9, "#2a9d8f")
    draw_legend(fig, [("#2a9d8f", "Search efficiency", True),
                      ("#777777", "Tham chiếu 100%", False)])
    fig.save("efficiency", output)


def convergence_chart(raw, root, output):
    selected = [r for r in raw if r["run_kind"] == "measurement" and int(r["seed"]) == 11]
    histories = {}
    for threads in THREADS:
        record = next(r for r in selected if int(r["requested_threads"]) == threads)
        rows = csv_rows(root / record["history_path"])
        histories[threads] = [(int(row["iteration"]), int(row["best_length"])) for row in rows]
    all_y = [y for rows in histories.values() for _, y in rows]
    ymin, ymax = min(all_y), max(all_y)
    pad = max(1, (ymax-ymin)*0.08)
    ymin, ymax = ymin-pad, ymax+pad
    fig = Figure()
    fig.text("Hội tụ global best — seed 11", W/2, 38, 38, bold=True, align="center")
    fig.text("Lấy từ history CSV; nếu đường trùng nhau, marker phân biệt tại các vòng khác nhau",
             W/2, 92, 21, color="#555555", align="center")
    fig.text("Độ dài tour (đơn vị khoảng cách)", LEFT, 126, 22, bold=True)
    fig.text("Vòng lặp", (LEFT+RIGHT)/2, 975, 22, bold=True, align="center")
    for i in range(6):
        value = ymin+(ymax-ymin)*i/5
        py = BOTTOM-(value-ymin)/(ymax-ymin)*(BOTTOM-TOP)
        fig.line([(LEFT, py), (RIGHT, py)], "#d9dfe5", 1)
        fig.text(f"{value:,.0f}", LEFT-18, py-14, 18, color="#555555", align="right")
    fig.line([(LEFT, TOP), (LEFT, BOTTOM), (RIGHT, BOTTOM)], "#333333", 2)
    max_iteration = max(x for rows in histories.values() for x, _ in rows)
    xmap = lambda iteration: LEFT+iteration/max_iteration*(RIGHT-LEFT)
    ymap = lambda value: BOTTOM-(value-ymin)/(ymax-ymin)*(BOTTOM-TOP)
    for tick in range(0, max_iteration+1, 40):
        px = xmap(tick)
        fig.line([(px, BOTTOM), (px, BOTTOM+8)], "#333333", 2)
        fig.text(str(tick), px, BOTTOM+18, 18, align="center")
    styles = {1: (None, 0), 2: (14, 0), 4: (5, 0), 8: (None, 0)}
    markers = {1: "circle", 2: "square", 4: "diamond", 8: "triangle"}
    for threads in THREADS:
        rows = histories[threads]
        points = [(xmap(x), ymap(value)) for x, value in rows]
        fig.line(points, COLORS[threads], 4, styles[threads][0])
        # Stagger marker locations to expose coincident curves without moving data.
        for index in range(threads % 4, len(points), 40):
            px, py = points[index]
            if markers[threads] == "square":
                fig.rect(px-6, py-6, px+6, py+6, COLORS[threads])
            elif markers[threads] == "diamond":
                fig.line([(px,py-8),(px+8,py),(px,py+8),(px-8,py),(px,py-8)], COLORS[threads], 3)
            elif markers[threads] == "triangle":
                fig.line([(px,py-8),(px+8,py+7),(px-8,py+7),(px,py-8)], COLORS[threads], 3)
            else:
                fig.circle(px, py, 6, "white", COLORS[threads], 3)
    fig.text("Các marker đặt tại vòng gốc; không dịch chuyển đường dữ liệu.",
             W/2, 1005, 18, color="#555555", align="center")
    draw_legend(fig, [(COLORS[t], f"{t} luồng", True) for t in THREADS], y=1045)
    fig.save("convergence", output)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--results", required=True, help="Directory containing raw.csv and metadata.json")
    args = ap.parse_args()
    root = Path(args.results).resolve()
    raw = csv_rows(root / "raw.csv")
    summary = csv_rows(root / "summary.csv")
    if not raw or not summary:
        ap.error("raw.csv and summary.csv must contain benchmark data")
    output = root / "figures"
    output.mkdir(exist_ok=True)
    execution_chart(raw, summary, output)
    speedup_chart(summary, output)
    efficiency_chart(summary, output)
    convergence_chart(raw, root, output)
    print(f"Created 4 PNG (220 dpi) and SVG charts under {output}")


if __name__ == "__main__":
    main()
