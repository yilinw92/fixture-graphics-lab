"""Integration checks verify actual CLI exit codes and exported evidence."""
from pathlib import Path
import argparse
import json
import subprocess

parser=argparse.ArgumentParser()
parser.add_argument("binary",type=Path)
args=parser.parse_args()
binary=args.binary.resolve()
for scene,code,count in [("nominal",0,0),("edge_failure",2,1),("overlap_failure",2,1)]:
    output=Path("artifacts/integration")/scene
    done=subprocess.run([str(binary),"--scene",f"examples/{scene}.scene","--out",str(output)],capture_output=True,text=True)
    assert done.returncode==code,(scene,done.stdout,done.stderr)
    data=json.loads((output/"report.json").read_text())
    assert len(data["findings"])==count
    assert bool(data["quote"])==(code==0)
    assert (output/"preview.svg").read_text().startswith('<svg')
    print(f"PASS: {scene}, exit={code}, findings={len(data['findings'])}")
invalid=subprocess.run([str(binary),"--quantity","0"],capture_output=True)
assert invalid.returncode==1
print("PASS: invalid quantity rejected")
