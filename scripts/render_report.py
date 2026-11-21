"""Render REPORT.md to the matching submission PDF."""

from pathlib import Path
import gc

import fitz
from markdown_it import MarkdownIt


ROOT = Path(__file__).resolve().parent.parent
source = ROOT / "REPORT.md"
target = ROOT / "REPORT.pdf"
html = MarkdownIt("commonmark").enable("table").render(source.read_text(encoding="utf-8"))
css = """
body { font-family: Arial, sans-serif; font-size: 10.5pt; line-height: 1.4; color: #17212b; }
h1 { font-size: 18pt; color: #143c5b; margin: 0 0 12pt; }
h2 { font-size: 14pt; color: #143c5b; margin: 18pt 0 7pt; }
h3 { font-size: 11.5pt; color: #245c7c; margin: 12pt 0 5pt; }
p { margin: 0 0 9pt; }
table { border-collapse: collapse; width: 100%; font-size: 8.5pt; margin: 8pt 0 12pt; }
th, td { border: 0.5pt solid #b3c2ce; padding: 3pt 4pt; vertical-align: top; }
th { background: #e8f0f5; }
img { width: 440pt; max-width: 100%; height: auto; }
code { font-family: Courier, monospace; font-size: 8pt; }
a { color: #155e8a; }
"""
story = fitz.Story(html=html, user_css=css, archive=fitz.Archive(str(ROOT)))
writer = fitz.DocumentWriter(str(target))
page = fitz.Rect(0, 0, 595, 842)
body = fitz.Rect(50, 52, 545, 801)
while True:
    more, _ = story.place(body)
    device = writer.begin_page(page)
    story.draw(device)
    writer.end_page()
    if not more:
        break
writer.close()
del writer
del story
gc.collect()

document = fitz.open(target)
for number, sheet in enumerate(document, 1):
    sheet.insert_text((46, 815), "CS 253 / MA 253  |  Project 02", fontsize=8, color=(0.25, 0.35, 0.45))
    sheet.insert_text((530, 815), str(number), fontsize=8, color=(0.25, 0.35, 0.45))
compressed = ROOT / "build" / "report-compressed.pdf"
compressed.parent.mkdir(exist_ok=True)
document.save(compressed, garbage=4, deflate=True, deflate_images=True)
document.close()
del document
gc.collect()
compressed.replace(target)
document = fitz.open(target)
print(f"Rendered {source.name} to {target.name}: {len(document)} pages")
