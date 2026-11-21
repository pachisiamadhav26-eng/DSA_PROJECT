"""Render the authored D6 Markdown draft to a readable PDF. Requires ReportLab."""

from html import escape
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "build/bench/python"))

from reportlab.lib import colors
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.platypus import SimpleDocTemplate, Paragraph, KeepTogether, PageBreak


def main():
    directory = Path(__file__).resolve().parent
    styles = getSampleStyleSheet()
    styles.add(ParagraphStyle(name="BodyD6", fontName="Helvetica", fontSize=9.5,
                              leading=12.2, spaceAfter=6, alignment=TA_LEFT))
    styles["Heading1"].fontSize = 17
    styles["Heading1"].leading = 21
    styles["Heading1"].spaceAfter = 10
    styles["Heading2"].fontSize = 11
    styles["Heading2"].leading = 14
    styles["Heading2"].spaceBefore = 7
    styles["Heading2"].spaceAfter = 5
    text = (directory / "D6.md").read_text(encoding="utf-8").replace("—", "-").replace("–", "-")
    story = []
    headings = []
    for block in text.split("\n\n"):
        block = block.strip()
        if not block:
            continue
        if block == "<!-- pagebreak -->":
            story.extend(headings)
            headings = []
            story.append(PageBreak())
            continue
        style = styles["BodyD6"]
        if block.startswith("# "):
            block, style = block[2:], styles["Heading1"]
        elif block.startswith("## "):
            block, style = block[3:], styles["Heading2"]
        formatted = escape(block.replace("\n", " "))
        formatted = re.sub(r"`([^`]+)`", r'<font name="Courier">\1</font>', formatted)
        paragraph = Paragraph(formatted, style)
        if block and style in (styles["Heading1"], styles["Heading2"]):
            headings.append(paragraph)
        else:
            story.append(KeepTogether(headings + [paragraph]) if headings else paragraph)
            headings = []
    story.extend(headings)

    def footer(canvas, document):
        canvas.setFont("Helvetica", 9)
        canvas.setFillColor(colors.HexColor("#555555"))
        canvas.drawString(44, 27, f"Project 2 | D6 | Sumit | Roll {roll_number} | Cost predictions")
        canvas.drawRightString(document.pagesize[0] - 48, 30, str(document.page))

    match = re.search(r"Roll number: (\d+)", text)
    if match is None:
        raise ValueError("D6.md must name its author's roll number")
    roll_number = match.group(1)
    output = directory / f"D6_{roll_number}.pdf"
    document = SimpleDocTemplate(str(output), pagesize=(595.28, 841.89), leftMargin=44, rightMargin=44,
                      topMargin=35, bottomMargin=43, title="D6 - Leaderboard cost predictions",
                      author=f"Sumit ({roll_number})")
    document.build(story, onFirstPage=footer, onLaterPages=footer)
    if document.page > 2:
        output.unlink()
        raise ValueError("D6 exceeds the submission limit of two pages; shorten D6.md")
    print(output)


if __name__ == "__main__":
    main()
