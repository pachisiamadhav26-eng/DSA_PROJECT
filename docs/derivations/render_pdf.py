"""Render the derivation Markdown files using ReportLab."""

import argparse
from pathlib import Path
from xml.sax.saxutils import escape

from reportlab.lib import colors
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm
from reportlab.platypus import Paragraph, SimpleDocTemplate


def render(source: Path) -> None:
    body = ParagraphStyle("Body", fontName="Helvetica", fontSize=10.5, leading=14,
                          spaceAfter=8, alignment=TA_LEFT)
    title = ParagraphStyle("Title", parent=body, fontName="Helvetica-Bold",
                           fontSize=21, leading=25, spaceAfter=12, keepWithNext=True)
    heading = ParagraphStyle("Heading", parent=body, fontName="Helvetica-Bold",
                             fontSize=13, leading=16, spaceBefore=5, keepWithNext=True)
    sections = source.read_text(encoding="utf-8").strip().split("\n\n")
    story = []
    for section in sections:
        line = " ".join(part.strip() for part in section.splitlines())
        if line.startswith("# "):
            story.append(Paragraph(escape(line[2:]), title))
        elif line.startswith("## "):
            story.append(Paragraph(escape(line[3:]), heading))
        else:
            story.append(Paragraph(escape(line), body))

    def footer(canvas, doc):
        canvas.setFont("Helvetica", 9)
        canvas.setFillColor(colors.HexColor("#495566"))
        canvas.drawString(20 * mm, 14 * mm, "CS 253 / MA 253  |  Live Leaderboard")
        canvas.drawRightString(A4[0] - 20 * mm, 14 * mm, str(doc.page))

    document = SimpleDocTemplate(str(source.with_suffix(".pdf")), pagesize=A4,
                                 leftMargin=20 * mm, rightMargin=20 * mm,
                                 topMargin=17 * mm, bottomMargin=22 * mm,
                                 title=sections[0][2:])
    document.build(story, onFirstPage=footer, onLaterPages=footer)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sources", type=Path, nargs="+")
    for source in parser.parse_args().sources:
        render(source)
