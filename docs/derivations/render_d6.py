"""Render the authored D6 Markdown draft to a readable PDF. Requires ReportLab."""

from html import escape
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "build/bench/python"))

from reportlab.lib import colors
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.platypus import SimpleDocTemplate, Paragraph, KeepTogether


def main():
    directory = Path(__file__).resolve().parent
    styles = getSampleStyleSheet()
    styles.add(ParagraphStyle(name="BodyD6", fontName="Helvetica", fontSize=10,
                              leading=14, spaceAfter=8, alignment=TA_LEFT))
    styles["Heading1"].fontSize = 19
    styles["Heading1"].leading = 24
    styles["Heading1"].spaceAfter = 15
    styles["Heading2"].fontSize = 12
    styles["Heading2"].leading = 16
    styles["Heading2"].spaceBefore = 10
    styles["Heading2"].spaceAfter = 7
    text = (directory / "D6.md").read_text(encoding="utf-8").replace("—", "-").replace("–", "-")
    story = []
    headings = []
    for block in text.split("\n\n"):
        block = block.strip()
        if not block:
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
        canvas.drawString(48, 30, "Project 2 | D6 | Sumit | Cost predictions")
        canvas.drawRightString(document.pagesize[0] - 48, 30, str(document.page))

    output = directory / "D6.pdf"
    SimpleDocTemplate(str(output), pagesize=(595.28, 841.89), leftMargin=48, rightMargin=48,
                      topMargin=45, bottomMargin=49, title="D6 - Leaderboard cost predictions",
                      author="Sumit").build(story, onFirstPage=footer, onLaterPages=footer)
    print(output)


if __name__ == "__main__":
    main()
