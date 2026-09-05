import xml.etree.ElementTree as ET
from pathlib import Path

root = Path(__file__).resolve().parents[1]
files = [root / "user_client" / "userwindow.ui", root / "admin_client" / "adminwindow.ui"]
for file in files:
    tree = ET.parse(file)
    assert tree.getroot().tag == "ui"
    assert tree.find("class") is not None
    print("PASS", file.name)

