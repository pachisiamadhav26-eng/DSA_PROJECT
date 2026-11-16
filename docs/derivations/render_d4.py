"""Render the English D4.md proof into D4.pdf using ReportLab."""

from html import escape
from pathlib import Path
import re

from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm
from reportlab.platypus import PageBreak, Paragraph, Preformatted, SimpleDocTemplate, Table


HERE = Path(__file__).resolve().parent
INK = colors.HexColor("#19263A")
BLUE = colors.HexColor("#243E64")
MUTED = colors.HexColor("#526074")
RULE = colors.HexColor("#CFD8E3")
PANEL = colors.HexColor("#F1F5F9")

BODY = ParagraphStyle("Body", fontName="Helvetica", fontSize=10.5, leading=14,
                      textColor=INK, spaceAfter=7)
TITLE = ParagraphStyle("Title", parent=BODY, fontName="Helvetica-Bold", fontSize=23,
                       leading=28, textColor=BLUE, spaceAfter=11, keepWithNext=True)
HEADING = ParagraphStyle("Heading", parent=BODY, fontName="Helvetica-Bold", fontSize=13.2,
                         leading=17, textColor=BLUE, spaceBefore=7, spaceAfter=5,
                         keepWithNext=True)
META = ParagraphStyle("Meta", parent=BODY, textColor=MUTED, spaceAfter=5,
                      keepWithNext=True)
BULLET = ParagraphStyle("Bullet", parent=BODY, leftIndent=13, bulletIndent=2,
                        spaceAfter=5)
CODE = ParagraphStyle("Code", fontName="Courier", fontSize=9, leading=11.5,
                      textColor=INK)


def inline(text):
    """Apply only the inline styles used by this proof."""
    text = escape(text)
    text = re.sub(r"\*\*([^*\n]+)\*\*", r"<b>\1</b>", text)
    return re.sub(r"\x60([^\x60]+)\x60", r'<font name="Courier">\1</font>', text)


def diagram(lines, width):
    # Leave the original spaces in place so the branches line up.
    code = "\n".join(line[4:] for line in lines)
    panel = Table([[Preformatted(code, CODE)]], colWidths=[width], spaceAfter=8)
    panel.setStyle([
        ("BACKGROUND", (0, 0), (-1, -1), PANEL),
        ("BOX", (0, 0), (-1, -1), 0.5, RULE),
        ("LEFTPADDING", (0, 0), (-1, -1), 10),
        ("TOPPADDING", (0, 0), (-1, -1), 8),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 8),
    ])
    return panel


def draw_margins(canvas, doc):
    canvas.saveState()
    left = doc.leftMargin
    right = A4[0] - doc.rightMargin
    canvas.setStrokeColor(RULE)
    canvas.line(left, A4[1] - 13 * mm, right, A4[1] - 13 * mm)
    canvas.line(left, 17 * mm, right, 17 * mm)
    canvas.setFont("Helvetica", 8)
    canvas.setFillColor(MUTED)
    canvas.drawString(left, 12 * mm, "Om Dehariya  |  D4  |  Live Leaderboard")
    canvas.drawRightString(right, 12 * mm, str(doc.page))
    canvas.restoreState()


def main():
    proof = (HERE / "D4.md").read_text(encoding="utf-8")
    document = SimpleDocTemplate(
        str(HERE / "D4.pdf"), pagesize=A4,
        leftMargin=20 * mm, rightMargin=20 * mm, topMargin=18 * mm,
        bottomMargin=22 * mm, title="D4 AVL Subtree Sizes and Selection",
        author="Om Dehariya", invariant=True,
    )
    story = []
    for block in proof.strip().split("\n\n"):
        lines = block.splitlines()
        if block == "<!-- page break -->":
            story.append(PageBreak())
        elif block.startswith("# "):
            story.append(Paragraph(inline(block[2:]), TITLE))
        elif block.startswith("## "):
            story.append(Paragraph(inline(block[3:]), HEADING))
        elif block.startswith("    "):
            story.append(diagram(lines, document.width - 12))
        elif block.startswith("- "):
            for line in lines:
                story.append(Paragraph(inline(line[2:]), BULLET, bulletText="•"))
        else:
            style = BODY
            if block.startswith(("Om Dehariya |", "CS 253 / MA 253 |")):
                style = META
            story.append(Paragraph(inline(" ".join(lines)), style))

    document.build(story, onFirstPage=draw_margins, onLaterPages=draw_margins)


if __name__ == "__main__":
    main()
