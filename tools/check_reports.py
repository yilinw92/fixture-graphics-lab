"""Integration checks verify actual CLI exit codes and exported evidence."""
from pathlib import Path
import argparse
import json
import subprocess

parser=argparse.ArgumentParser()
parser.add_argument("binary",type=Path)
args=parser.parse_args()
binary=args.binary.resolve()
root=Path(__file__).resolve().parents[1]
for scene,code,count in [("nominal",0,0),("edge_failure",2,1),("overlap_failure",2,1)]:
    output=root/"artifacts/integration"/scene
    done=subprocess.run([str(binary),"--scene",f"examples/{scene}.scene","--out",str(output.relative_to(root))],cwd=root,capture_output=True,text=True)
    assert done.returncode==code,(scene,done.stdout,done.stderr)
    data=json.loads((output/"report.json").read_text())
    assert len(data["findings"])==count
    assert bool(data["quote"])==(code==0)
    assert (output/"preview.svg").read_text().startswith('<svg')
    print(f"PASS: {scene}, exit={code}, findings={len(data['findings'])}")
invalid=subprocess.run([str(binary),"--quantity","0"],cwd=root,capture_output=True)
assert invalid.returncode==1
print("PASS: invalid quantity rejected")
for name,expected in [("notched_fixture",0),("notch_violation",2),("inch_fixture",0),("invalid_open",1),("invalid_crossing",1),("invalid_bulge",1),("invalid_units",1)]:
    directory=root/"artifacts/integration"/name
    done=subprocess.run([str(binary),"--dxf",f"examples/{name}.dxf","--out",str(directory.relative_to(root))],cwd=root,capture_output=True,text=True)
    assert done.returncode==expected,(name,done.stdout,done.stderr)
    if expected!=1:
        data=json.loads((directory/"report.json").read_text())
        assert data["geometry_passes"]==(expected==0)
        assert (directory/"normalized.dxf").exists()
    print(f"PASS: DXF {name}, exit={expected}")
