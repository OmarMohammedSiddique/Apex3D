#!/usr/bin/env python3
"""Generate the Apex3D Checkpoint 2 evidence PDF from committed output images."""

from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.pagesizes import A4, landscape
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.utils import ImageReader
from reportlab.platypus import Paragraph
from reportlab.pdfgen import canvas


ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "output" / "pdf" / "Apex3D_Checkpoint2_Evidence.pdf"
PAGE_SIZE = landscape(A4)
PAGE_WIDTH, PAGE_HEIGHT = PAGE_SIZE
MARGIN = 42
ACCENT = colors.HexColor("#2563EB")
TEXT = colors.HexColor("#172033")
MUTED = colors.HexColor("#596579")
LIGHT = colors.HexColor("#E8EEF7")


EVIDENCE = [
    {
        "title": "1. Homogeneous 2D transformations",
        "image": ROOT / "checkpoint2/transformations/output/transformations_demo.png",
        "summary": (
            "A simplified Formula 1 car is transformed using handwritten 3 x 3 "
            "homogeneous matrices. The demonstration compares the original model "
            "with translation, rotation, scaling and a composed transform."
        ),
        "proof": "8 deterministic transformation tests pass.",
    },
    {
        "title": "2. Timeline and interpolation",
        "image": ROOT / "checkpoint2/animation/timeline/output/contact_sheet.png",
        "summary": (
            "A frame clock converts frame numbers into animation progress. Linear "
            "position interpolation moves the project component across 13 frames, "
            "with clamping, looping and shortest-arc angle interpolation tested."
        ),
        "proof": "23 deterministic timeline and interpolation checks pass.",
    },
    {
        "title": "3. Hierarchical wheel animation",
        "image": ROOT / "checkpoint2/animation/wheels/output/wheel_contact_sheet.png",
        "summary": (
            "Circle and ellipse components from Checkpoint 1 are animated as wheels "
            "and a telemetry pod. Parent translation and local wheel rotation are "
            "combined, including rolling distance equal to radius times angle."
        ),
        "proof": "17 hierarchy, geometry and rendering checks pass.",
    },
    {
        "title": "4. Keyframed polygon car",
        "image": ROOT / "checkpoint2/animation/car/output/car_contact_sheet.png",
        "summary": (
            "Thirteen polygons form a simplified Formula 1 car. One shared matrix "
            "animates the complete model through translation, centre-pivot rotation, "
            "non-uniform scaling and combined motion across 25 frames."
        ),
        "proof": "11 pose, geometry, raster and PNG checks pass.",
    },
    {
        "title": "5. Catmull-Rom spline motion",
        "image": ROOT / "checkpoint2/animation/splines/output/spline_contact_sheet.png",
        "summary": (
            "A closed Catmull-Rom spline represents a simplified circuit path. An "
            "analytic tangent determines the car heading so the vehicle follows and "
            "faces along the curve throughout a 33-frame lap."
        ),
        "proof": "All spline motion checks pass for 8 segments and 33 poses.",
    },
    {
        "title": "6. Cubic Bezier path animation",
        "image": ROOT / "checkpoint2/animation/bezier/output/bezier_contact_sheet.png",
        "summary": (
            "Eight C1-continuous cubic Bezier segments define a closed circuit. An "
            "analytic derivative controls heading, while an arc-length lookup table "
            "keeps the car's 25-frame travel approximately uniform."
        ),
        "proof": "14 curve, continuity, motion, pose and rendering checks pass.",
    },
]


def draw_header(pdf: canvas.Canvas, title: str, page_number: int) -> None:
    pdf.setFillColor(TEXT)
    pdf.setFont("Helvetica-Bold", 20)
    pdf.drawString(MARGIN, PAGE_HEIGHT - 48, title)
    pdf.setStrokeColor(ACCENT)
    pdf.setLineWidth(2)
    pdf.line(MARGIN, PAGE_HEIGHT - 60, PAGE_WIDTH - MARGIN, PAGE_HEIGHT - 60)
    pdf.setFillColor(MUTED)
    pdf.setFont("Helvetica", 9)
    pdf.drawRightString(
        PAGE_WIDTH - MARGIN,
        24,
        f"Apex3D - Checkpoint 2 evidence | Page {page_number}",
    )


def draw_wrapped_text(
    pdf: canvas.Canvas,
    text: str,
    x: float,
    y: float,
    width: float,
    font_size: int = 11,
    leading: int = 15,
) -> float:
    style = ParagraphStyle(
        "body",
        parent=getSampleStyleSheet()["BodyText"],
        fontName="Helvetica",
        fontSize=font_size,
        leading=leading,
        textColor=TEXT,
    )
    paragraph = Paragraph(text, style)
    _, height = paragraph.wrap(width, 200)
    paragraph.drawOn(pdf, x, y - height)
    return y - height


def draw_fitted_image(
    pdf: canvas.Canvas,
    image_path: Path,
    x: float,
    y: float,
    max_width: float,
    max_height: float,
) -> None:
    if not image_path.exists():
        raise FileNotFoundError(f"Missing evidence image: {image_path}")
    image = ImageReader(str(image_path))
    width, height = image.getSize()
    scale = min(max_width / width, max_height / height)
    draw_width = width * scale
    draw_height = height * scale
    image_x = x + (max_width - draw_width) / 2
    image_y = y + (max_height - draw_height) / 2
    pdf.setStrokeColor(LIGHT)
    pdf.setLineWidth(1)
    pdf.rect(x, y, max_width, max_height, stroke=1, fill=0)
    pdf.drawImage(
        image,
        image_x,
        image_y,
        width=draw_width,
        height=draw_height,
        preserveAspectRatio=True,
        mask="auto",
    )


def cover_page(pdf: canvas.Canvas) -> None:
    pdf.setFillColor(TEXT)
    pdf.setFont("Helvetica-Bold", 30)
    pdf.drawString(MARGIN, PAGE_HEIGHT - 92, "Apex3D")
    pdf.setFont("Helvetica-Bold", 20)
    pdf.drawString(MARGIN, PAGE_HEIGHT - 126, "Checkpoint 2 - Transformation and Animation")
    pdf.setStrokeColor(ACCENT)
    pdf.setLineWidth(3)
    pdf.line(MARGIN, PAGE_HEIGHT - 145, PAGE_WIDTH - MARGIN, PAGE_HEIGHT - 145)

    y = PAGE_HEIGHT - 185
    y = draw_wrapped_text(
        pdf,
        "This evidence pack demonstrates how the group applied transformations and "
        "animation in Pure C++20 to simplified Formula 1 components developed during "
        "Checkpoint 1. All images were generated by the repository's CPU-rendered "
        "demonstration programs.",
        MARGIN,
        y,
        PAGE_WIDTH - (2 * MARGIN),
        12,
        17,
    )

    pdf.setFillColor(colors.HexColor("#F5F7FB"))
    pdf.roundRect(MARGIN, 112, PAGE_WIDTH - (2 * MARGIN), 205, 6, stroke=0, fill=1)
    pdf.setFillColor(TEXT)
    pdf.setFont("Helvetica-Bold", 14)
    pdf.drawString(MARGIN + 20, 290, "Implemented techniques")

    items = [
        "3 x 3 homogeneous matrices and composed 2D transformations",
        "Frame timing, keyframes and position/angle interpolation",
        "Hierarchical wheel rotation and rolling without slipping",
        "Translation, rotation and scaling of a multi-polygon car",
        "Catmull-Rom spline motion with tangent-based orientation",
        "Cubic Bezier motion with approximate arc-length sampling",
    ]
    item_y = 264
    for item in items:
        pdf.setFillColor(ACCENT)
        pdf.circle(MARGIN + 27, item_y + 3, 2.5, stroke=0, fill=1)
        pdf.setFillColor(TEXT)
        pdf.setFont("Helvetica", 11)
        pdf.drawString(MARGIN + 40, item_y, item)
        item_y -= 25

    pdf.setFillColor(MUTED)
    pdf.setFont("Helvetica", 10)
    pdf.drawString(MARGIN, 72, "Group: Omar Siddique, Hetvi Barot, Azhar Ahmed, Nyariki, Casper Muhereza, Shadrack Mungai")
    pdf.drawString(MARGIN, 52, "Verification: checkpoint2/verify_all.sh - all strict builds, tests and demonstrations pass")
    pdf.drawRightString(PAGE_WIDTH - MARGIN, 24, "Apex3D - Checkpoint 2 evidence | Page 1")
    pdf.showPage()


def evidence_page(pdf: canvas.Canvas, item: dict[str, object], page_number: int) -> None:
    draw_header(pdf, str(item["title"]), page_number)
    draw_fitted_image(
        pdf,
        Path(item["image"]),
        MARGIN,
        145,
        PAGE_WIDTH - (2 * MARGIN),
        PAGE_HEIGHT - 225,
    )
    text_y = 128
    text_y = draw_wrapped_text(
        pdf,
        str(item["summary"]),
        MARGIN,
        text_y,
        PAGE_WIDTH - (2 * MARGIN),
        10,
        14,
    )
    pdf.setFillColor(ACCENT)
    pdf.setFont("Helvetica-Bold", 10)
    pdf.drawString(MARGIN, text_y - 17, str(item["proof"]))
    pdf.showPage()


def build_pdf() -> None:
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    pdf = canvas.Canvas(str(OUTPUT), pagesize=PAGE_SIZE, pageCompression=1)
    pdf.setTitle("Apex3D Checkpoint 2 Evidence")
    pdf.setAuthor("Apex3D Group")
    cover_page(pdf)
    for page_number, item in enumerate(EVIDENCE, start=2):
        evidence_page(pdf, item, page_number)
    pdf.save()
    print(OUTPUT)


if __name__ == "__main__":
    build_pdf()

