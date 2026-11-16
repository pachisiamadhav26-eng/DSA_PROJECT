"""Rebuild D4 PDF with reportlab: python docs/derivations/render_d4.py.

The Markdown proof is the editable source. Output uses only built-in PDF fonts.
"""
from pathlib import Path
from html import escape
from reportlab.lib import colors
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.units import mm
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Preformatted, PageBreak

folder = Path(__file__).resolve().parent
styles = getSampleStyleSheet()
styles.add(ParagraphStyle('ProofBody', fontName='Helvetica', fontSize=10,
                          leading=14, spaceAfter=7, alignment=TA_LEFT))
styles['Title'].fontName = 'Helvetica-Bold'
styles['Title'].fontSize = 18
styles['Title'].leading = 23
styles['Heading2'].fontSize = 12
styles['Heading2'].leading = 16
styles['Heading2'].spaceBefore = 12
styles['Heading2'].textColor = colors.black
proof = (folder / 'D4-om.md').read_text(encoding='utf-8')
story = []
# Each blank-line-separated block becomes a title, example, or paragraph.
for block in proof.strip().split('\n\n'):
    if block.startswith('# '):
        story.append(Paragraph(escape(block[2:]), styles['Title']))
    elif block.startswith('## '):
        if block.startswith('## Correctness and cost of select'):
            story.append(PageBreak())
        story.append(Paragraph(escape(block[3:]), styles['Heading2']))
    elif block.startswith('    '):
        example_lines = [line[4:] for line in block.splitlines()]
        example = '\n'.join(example_lines)
        story.append(Preformatted(example, styles['Code']))
        story.append(Spacer(1, 5))
    else:
        lines = block.splitlines()
        if block.startswith('1.'):
            # Preserve the line breaks between the three select cases.
            text = '<br/>'.join(escape(line) for line in lines)
        else:
            text = escape(' '.join(lines))
        story.append(Paragraph(text, styles['ProofBody']))

def footer(canvas, doc):
    canvas.saveState()
    canvas.setFont('Helvetica', 8)
    canvas.setFillColor(colors.HexColor('#555555'))
    canvas.drawString(20*mm, 13*mm, 'D4 | Om Dehariya | Project 2')
    canvas.drawRightString(190*mm, 13*mm, str(doc.page))
    canvas.restoreState()

doc = SimpleDocTemplate(str(folder / 'D4-om.pdf'), pagesize=(210*mm, 297*mm),
                        rightMargin=20*mm, leftMargin=20*mm, topMargin=18*mm,
                        bottomMargin=22*mm, title='D4 Subtree sizes and select',
                        author='Om Dehariya')
doc.build(story, onFirstPage=footer, onLaterPages=footer)
