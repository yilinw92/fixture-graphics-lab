"""Reproducible, synthetic DXF test drawings; no employer CAD data."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1] / "examples"
def drawing(vertices, holes, units=4, bulge=False, closed=True):
    tags=[(0,"SECTION"),(2,"HEADER"),(9,"$INSUNITS"),(70,units),(0,"ENDSEC"),(0,"SECTION"),(2,"ENTITIES"),(0,"LWPOLYLINE"),(90,len(vertices)),(70,int(closed))]
    for x,y in vertices:
        tags += [(10,x),(20,y)]
        if bulge: tags.append((42,0.2))
    for x,y,r in holes: tags += [(0,"CIRCLE"),(10,x),(20,y),(40,r)]
    tags += [(0,"ENDSEC"),(0,"EOF")]
    return "".join(f"{code}\n{value}\n" for code,value in tags)
outline=[(0,0),(140,0),(140,90),(95,90),(95,55),(45,55),(45,90),(0,90)]
holes=[(18,18,6),(122,18,6),(20,70,6),(120,70,6),(60,28,8),(82,28,8)]
ROOT.mkdir(exist_ok=True)
for name, content in {
    "notched_fixture.dxf":drawing(outline,holes),
    "notch_violation.dxf":drawing(outline,holes+[(70,51,6)]),
    "inch_fixture.dxf":drawing([(10,20),(12,20),(12,21),(10,21)],[(10.5,20.5,0.1)],1),
    "invalid_bulge.dxf":drawing(outline,holes,bulge=True),
    "invalid_crossing.dxf":drawing([(0,0),(100,80),(0,80),(100,0)],[]),
    "invalid_open.dxf":drawing(outline,holes,closed=False),
    "invalid_units.dxf":drawing(outline,holes,units=0),
}.items(): (ROOT/name).write_text(content,encoding="utf-8")
