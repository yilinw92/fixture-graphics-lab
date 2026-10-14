# Validation record

Validated on 2026-10-13 (America/New_York). Independent synthetic examples; no manufacturing approval or commercial pricing claim.

| Check | Result |
| --- | --- |
| Windows GCC build | CLI, tests and viewer compiled with GCC 16.2.0 / C++17 |
| Core checks | 20 checks passed: nominal and invalid geometry, equality boundary, nonfinite inputs, duplicates, quote arithmetic, transforms and picking |
| CLI/export integration | Nominal returned 0; edge and overlap failures returned 2; invalid quantity returned 1; JSON/SVG outputs checked |
| Native OpenGL context | OpenGL 3.3.0, Intel Iris Xe Graphics, driver build 30.0.101.1960 |
| Native render and GPU readback | Shader compilation, draw and PPM capture succeeded |

`docs/media/opengl_edge.png` is converted from the actual framebuffer capture. `docs/media/nominal_report.json` and `edge_report.json` are generated CLI evidence.

The renderer uses triangles/lines for display; the independent analytical geometry core decides rule results. A graphically plausible picture is not used as mathematical proof.

GitHub Actions builds Linux and Windows and runs a software-rendered Linux capture. Local results do not imply that a future remote workflow has passed.
