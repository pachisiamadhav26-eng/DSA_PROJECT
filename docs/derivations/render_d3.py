from html import escape
from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import Paragraph, SimpleDocTemplate


root = Path(__file__).resolve().parent
source = root / "D3.md"
output = root / "D3.pdf"

title_style = ParagraphStyle(
    "Title",
    fontName="Helvetica-Bold",
    fontSize=16,
    leading=20,
    textColor=colors.HexColor("#17324d"),
    spaceAfter=9,
    alignment=TA_LEFT,
)
author_style = ParagraphStyle(
    "Author",
    fontName="Helvetica",
    fontSize=9,
    leading=13,
    textColor=colors.HexColor("#4c5c6b"),
    spaceAfter=16,
)
heading_style = ParagraphStyle(
    "Heading",
    fontName="Helvetica-Bold",
    fontSize=10.5,
    leading=14,
    textColor=colors.HexColor("#17324d"),
    spaceBefore=11,
    spaceAfter=4,
)
body_style = ParagraphStyle(
    "Body",
    fontName="Helvetica",
    fontSize=9.3,
    leading=14,
    textColor=colors.HexColor("#1c2730"),
    spaceAfter=7,
)

story = []
for raw_line in source.read_text(encoding="utf-8").splitlines():
    line = raw_line.strip()
    if not line:
        continue
    if line.startswith("# "):
        story.append(Paragraph(escape(line[2:]), title_style))
    elif line.startswith("## "):
        story.append(Paragraph(escape(line[3:]), heading_style))
    elif line.startswith("Author:"):
        story.append(Paragraph(escape(line), author_style))
    else:
        story.append(Paragraph(escape(line), body_style))


def page_frame(canvas, document):
    canvas.saveState()
    width, height = A4
    canvas.setStrokeColor(colors.HexColor("#b9c5cf"))
    canvas.setLineWidth(0.5)
    canvas.line(45, height - 39, width - 45, height - 39)
    canvas.setFont("Helvetica", 8)
    canvas.setFillColor(colors.HexColor("#4c5c6b"))
    canvas.drawString(45, height - 31, "CS 253 / MA 253 - Project 2")
    canvas.drawRightString(width - 45, height - 31, "D3")
    canvas.line(45, 39, width - 45, 39)
    canvas.drawString(45, 28, "Madhav Pachisia - Architect")
    canvas.drawRightString(width - 45, 28, f"Page {document.page}")
    canvas.restoreState()


document = SimpleDocTemplate(
    str(output),
    pagesize=A4,
    title="D3 - Competition rank in AVL and Fenwick leaderboards",
    author="Madhav Pachisia",
    subject="CS 253 / MA 253 Project 2 Architect derivation",
    leftMargin=45,
    rightMargin=45,
    topMargin=55,
    bottomMargin=52,
)
document.build(story, onFirstPage=page_frame, onLaterPages=page_frame)
print(output)
