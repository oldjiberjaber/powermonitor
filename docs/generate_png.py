import os
from PIL import Image, ImageDraw, ImageFont

# Canvas dimensions (2x scaling for crisp Retina display)
SCALE = 2
WIDTH = 1040 * SCALE
HEIGHT = 640 * SCALE

# Colors
BG_COLOR = (11, 15, 25)           # #0b0f19
CARD_BG = (18, 26, 44)            # #121a2c
CARD_BORDER = (51, 65, 85)        # #334155
HEADER_BG = (30, 41, 59)          # #1e293b

COLOR_RED = (239, 68, 68)         # #ef4444 (+24V)
COLOR_ORANGE = (249, 115, 22)     # #f97316 (Load +)
COLOR_3V3 = (251, 146, 60)        # #fb923c (3.3V Logic)
COLOR_VBUS = (56, 189, 248)       # #38bdf8 (VBUS Sense)
COLOR_GND = (148, 163, 184)       # #94a3b8 (0V GND)
COLOR_SDA = (0, 212, 255)         # #00d4ff (SDA Cyan)
COLOR_SCL = (34, 197, 94)         # #22c55e (SCL Green)
COLOR_TEXT_MAIN = (241, 245, 249) # #f1f5f9
COLOR_TEXT_MUTED = (148, 163, 184)# #94a3b8

img = Image.new('RGB', (WIDTH, HEIGHT), BG_COLOR)
draw = ImageDraw.Draw(img)

def s(val):
    return int(val * SCALE)

# Load fonts
try:
    font_title = ImageFont.truetype("arial.ttf", s(17))
    font_subtitle = ImageFont.truetype("arial.ttf", s(12))
    font_card_title = ImageFont.truetype("arialbd.ttf", s(11))
    font_bold = ImageFont.truetype("arialbd.ttf", s(11))
    font_regular = ImageFont.truetype("arial.ttf", s(10))
    font_small = ImageFont.truetype("arial.ttf", s(9))
except:
    font_title = font_subtitle = font_card_title = font_bold = font_regular = font_small = ImageFont.load_default()

# 1. Top Header Banner
draw.rounded_rectangle([s(30), s(15), s(1010), s(60)], radius=s(8), fill=HEADER_BG, outline=CARD_BORDER, width=s(1))
draw.text((s(50), s(26)), "24VDC Power Monitor & Enclosure Climate System", fill=COLOR_TEXT_MAIN, font=font_title)
draw.text((s(990), s(30)), "ESP32-S3 + INA226 + SHT30 System Schematic", fill=COLOR_VBUS, font=font_subtitle, anchor="ra")

def draw_box(x, y, w, h, title, title_color, border_color):
    draw.rounded_rectangle([s(x), s(y), s(x+w), s(y+h)], radius=s(10), fill=CARD_BG, outline=border_color, width=s(2))
    draw.rounded_rectangle([s(x), s(y), s(x+w), s(y+28)], radius=s(10), fill=(border_color[0]//4, border_color[1]//4, border_color[2]//4))
    draw.rectangle([s(x), s(y+16), s(x+w), s(y+28)], fill=(border_color[0]//4, border_color[1]//4, border_color[2]//4))
    draw.text((s(x + w//2), s(y + 14)), title, fill=title_color, font=font_card_title, anchor="mm")

# Box 1: 24V DC Power Supply (Top Left)
draw_box(40, 85, 200, 150, "24V DC POWER SOURCE", (252, 165, 165), COLOR_RED)
draw.text((s(140), s(135)), "24V DC PSU / Battery", fill=COLOR_TEXT_MAIN, font=font_bold, anchor="mm")
draw.text((s(140), s(155)), "Main Power Input", fill=COLOR_TEXT_MUTED, font=font_regular, anchor="mm")

draw.circle((s(215), s(185)), s(6), fill=COLOR_RED)
draw.text((s(200), s(185)), "+24V", fill=(252, 165, 165), font=font_bold, anchor="rm")

draw.circle((s(215), s(215)), s(6), fill=COLOR_GND)
draw.text((s(200), s(215)), "0V (GND)", fill=COLOR_GND, font=font_bold, anchor="rm")


# Box 2: INA226 Sensor Module (Top Middle)
draw_box(360, 85, 300, 240, "INA226 POWER SENSOR (0x40)", (199, 210, 254), (129, 140, 248))

# Shunt Graphic in center
draw.rounded_rectangle([s(460), s(130), s(560), s(165)], radius=s(6), fill=(49, 46, 129), outline=(165, 180, 252), width=s(1))
draw.text((s(510), s(147)), "10mΩ (R010)", fill=(224, 231, 255), font=font_bold, anchor="mm")

# IN+ & IN- on Top sides of Shunt
draw.circle((s(385), s(147)), s(7), fill=COLOR_RED)
draw.text((s(400), s(147)), "IN+", fill=(252, 165, 165), font=font_bold, anchor="lm")

draw.circle((s(635), s(147)), s(7), fill=COLOR_ORANGE)
draw.text((s(620), s(147)), "IN-", fill=(253, 186, 116), font=font_bold, anchor="rm")

# VBUS, VCC, GND, SCL, SDA along bottom/side pins
draw.circle((s(385), s(190)), s(6), fill=COLOR_VBUS)
draw.text((s(400), s(190)), "VBUS (Sense)", fill=COLOR_VBUS, font=font_regular, anchor="lm")

draw.circle((s(385), s(230)), s(6), fill=COLOR_3V3)
draw.text((s(400), s(230)), "VCC (3.3V)", fill=COLOR_3V3, font=font_regular, anchor="lm")

draw.circle((s(385), s(270)), s(6), fill=COLOR_GND)
draw.text((s(400), s(270)), "GND", fill=COLOR_GND, font=font_regular, anchor="lm")

draw.circle((s(540), s(305)), s(6), fill=COLOR_SDA)
draw.text((s(540), s(290)), "SDA", fill=COLOR_SDA, font=font_bold, anchor="mm")

draw.circle((s(600), s(305)), s(6), fill=COLOR_SCL)
draw.text((s(600), s(290)), "SCL", fill=COLOR_SCL, font=font_bold, anchor="mm")


# Box 3: 24V Load (Top Right)
draw_box(780, 85, 200, 150, "24V LOAD APPLIANCE", (243, 232, 255), (168, 85, 247))
draw.text((s(880), s(135)), "Telescope / Heaters", fill=COLOR_TEXT_MAIN, font=font_bold, anchor="mm")
draw.text((s(880), s(155)), "Monitored Load", fill=COLOR_TEXT_MUTED, font=font_regular, anchor="mm")

draw.circle((s(805), s(185)), s(6), fill=COLOR_ORANGE)
draw.text((s(820), s(185)), "+24V (In)", fill=(253, 186, 116), font=font_bold, anchor="lm")

draw.circle((s(805), s(215)), s(6), fill=COLOR_GND)
draw.text((s(820), s(215)), "0V Return", fill=COLOR_GND, font=font_bold, anchor="lm")


# Box 4: ESP32-S3 Controller (Bottom Left)
draw_box(40, 390, 370, 220, "ESP32-S3 & FreeRTOS SMP", (186, 230, 253), (56, 189, 248))
draw.rounded_rectangle([s(55), s(435), s(130), s(465)], radius=s(4), fill=(3, 105, 161))
draw.text((s(92), s(450)), "USB CDC", fill=(255,255,255), font=font_bold, anchor="mm")

draw.text((s(60), s(490)), "• Core 1: 50Hz ADC & Transients", fill=COLOR_TEXT_MUTED, font=font_regular)
draw.text((s(60), s(515)), "• Core 0: Web, MQTT & Captive Portal", fill=COLOR_TEXT_MUTED, font=font_regular)
draw.text((s(60), s(540)), "• NVS Flash Energy Accumulator", fill=COLOR_TEXT_MUTED, font=font_regular)
draw.text((s(60), s(575)), "Native USB Flashing (--no-stub)", fill=COLOR_VBUS, font=font_bold)

draw.circle((s(390), s(440)), s(6), fill=COLOR_3V3)
draw.text((s(375), s(440)), "3.3V (VCC Out)", fill=COLOR_3V3, font=font_bold, anchor="rm")

draw.circle((s(390), s(480)), s(6), fill=COLOR_GND)
draw.text((s(375), s(480)), "GND", fill=COLOR_GND, font=font_bold, anchor="rm")

draw.circle((s(390), s(525)), s(6), fill=COLOR_SDA)
draw.text((s(375), s(525)), "GPIO 8 (SDA)", fill=COLOR_SDA, font=font_bold, anchor="rm")

draw.circle((s(390), s(570)), s(6), fill=COLOR_SCL)
draw.text((s(375), s(570)), "GPIO 9 (SCL)", fill=COLOR_SCL, font=font_bold, anchor="rm")


# Box 5: SHT30 Climate Sensor (Bottom Right)
draw_box(630, 390, 350, 135, "SHT30 CLIMATE SENSOR (0x44 / 0x45)", (167, 243, 208), (16, 185, 129))
draw.circle((s(655), s(435)), s(6), fill=COLOR_3V3)
draw.text((s(670), s(435)), "VCC (3.3V)", fill=COLOR_3V3, font=font_regular, anchor="lm")

draw.circle((s(655), s(465)), s(6), fill=COLOR_GND)
draw.text((s(670), s(465)), "GND", fill=COLOR_GND, font=font_regular, anchor="lm")

draw.circle((s(655), s(495)), s(6), fill=COLOR_SDA)
draw.text((s(670), s(495)), "SDA", fill=COLOR_SDA, font=font_bold, anchor="lm")

draw.circle((s(765), s(495)), s(6), fill=COLOR_SCL)
draw.text((s(780), s(495)), "SCL", fill=COLOR_SCL, font=font_bold, anchor="lm")

draw.text((s(960), s(440)), "Enclosure Temp & Humidity", fill=COLOR_TEXT_MUTED, font=font_regular, anchor="rm")
draw.text((s(960), s(465)), "Condensation Safety Margin", fill=(52, 211, 153), font=font_bold, anchor="rm")


# ==================== WIRES ROUTING (Clean, non-crossing channels) ====================

# 1. +24V Supply to INA226 IN+
draw.line([s(215), s(185), s(290), s(185), s(290), s(147), s(385), s(147)], fill=COLOR_RED, width=s(3))
# Branch +24V to VBUS Sense Lead
draw.line([s(290), s(185), s(290), s(190), s(385), s(190)], fill=COLOR_VBUS, width=s(2))
draw.circle((s(290), s(185)), s(4), fill=COLOR_RED)

# 2. INA226 IN- to 24V Load +
draw.line([s(635), s(147), s(720), s(147), s(720), s(185), s(805), s(185)], fill=COLOR_ORANGE, width=s(3))

# 3. Ground Bus Return (0V Rail across bottom of power cards)
draw.line([s(805), s(215), s(720), s(215), s(720), s(270), s(215), s(270), s(215), s(215)], fill=COLOR_GND, width=s(3))
# Branch GND to INA226
draw.line([s(350), s(270), s(385), s(270)], fill=COLOR_GND, width=s(2))
draw.circle((s(350), s(270)), s(4), fill=COLOR_GND)
# Vertical GND trunk to bottom sensors
draw.line([s(450), s(270), s(450), s(480), s(390), s(480)], fill=COLOR_GND, width=s(2))
draw.line([s(450), s(465), s(655), s(465)], fill=COLOR_GND, width=s(2))
draw.circle((s(450), s(270)), s(4), fill=COLOR_GND)
draw.circle((s(450), s(465)), s(4), fill=COLOR_GND)

# 4. 3.3V Logic Power Trunk (ESP32 to INA226 and SHT30)
draw.line([s(390), s(440), s(480), s(440), s(480), s(230), s(385), s(230)], fill=COLOR_3V3, width=s(2))
draw.line([s(480), s(435), s(655), s(435)], fill=COLOR_3V3, width=s(2))
draw.circle((s(480), s(435)), s(4), fill=COLOR_3V3)

# 5. I2C Bus: SDA (GPIO 8)
draw.line([s(390), s(525), s(540), s(525), s(540), s(305)], fill=COLOR_SDA, width=s(2))
draw.line([s(540), s(495), s(655), s(495)], fill=COLOR_SDA, width=s(2))
draw.circle((s(540), s(495)), s(4), fill=COLOR_SDA)

# 6. I2C Bus: SCL (GPIO 9)
draw.line([s(390), s(570), s(600), s(570), s(600), s(305)], fill=COLOR_SCL, width=s(2))
draw.line([s(600), s(570), s(765), s(570), s(765), s(495)], fill=COLOR_SCL, width=s(2))
draw.circle((s(600), s(570)), s(4), fill=COLOR_SCL)

# Legend Box (Bottom Right)
draw.rounded_rectangle([s(630), s(545), s(980), s(610)], radius=s(8), fill=HEADER_BG, outline=CARD_BORDER, width=s(1))
draw.text((s(645), s(555)), "LEGEND:", fill=COLOR_TEXT_MUTED, font=font_bold)

draw.line([s(645), s(580), s(675), s(580)], fill=COLOR_RED, width=s(3))
draw.text((s(682), s(575)), "+24V Rail", fill=COLOR_TEXT_MAIN, font=font_regular)

draw.line([s(755), s(580), s(785), s(580)], fill=COLOR_GND, width=s(3))
draw.text((s(792), s(575)), "0V (GND)", fill=COLOR_TEXT_MAIN, font=font_regular)

draw.line([s(865), s(580), s(895), s(580)], fill=COLOR_3V3, width=s(2))
draw.text((s(902), s(575)), "3.3V Logic", fill=COLOR_TEXT_MAIN, font=font_regular)

draw.line([s(645), s(600), s(675), s(600)], fill=COLOR_SDA, width=s(2))
draw.text((s(682), s(595)), "SDA (GPIO8)", fill=COLOR_TEXT_MAIN, font=font_regular)

draw.line([s(755), s(600), s(785), s(600)], fill=COLOR_SCL, width=s(2))
draw.text((s(792), s(595)), "SCL (GPIO9)", fill=COLOR_TEXT_MAIN, font=font_regular)

draw.line([s(865), s(600), s(895), s(600)], fill=COLOR_VBUS, width=s(2))
draw.text((s(902), s(595)), "VBUS Sense", fill=COLOR_TEXT_MAIN, font=font_regular)

os.makedirs("docs", exist_ok=True)
img.save("docs/wiring_diagram.png", "PNG", quality=95)
print("Regenerated docs/wiring_diagram.png cleanly!")
