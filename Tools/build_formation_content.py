"""Import the two new closed outcrop cores without rebuilding older assets."""
import json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Tools'))
from build_rock_content import build_rocks,audit_rocks
from build_rock_surface_content import build_rock_surfaces
geometry=json.loads((ROOT/'blender/exports/FormationRocks/Manifest.json').read_text(encoding='utf-8'))
surfaces=json.loads((ROOT/'Documentation/ROCK_FORMATION_ASSETS.json').read_text(encoding='utf-8'))
assert [a['name'] for a in geometry['assets']]==['SM_LakeRock4','SM_LakeRock5']
build_rocks(geometry,'FormationContentBuild.json')
build_rock_surfaces(surfaces,'FormationSurfaceBuild.json',4096)
audit_rocks(geometry,'FormationRenderAudit.json')
