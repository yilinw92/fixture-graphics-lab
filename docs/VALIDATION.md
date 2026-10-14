# Validation record — version 0.2

Validated on 2026-10-14, Windows 11, GCC 16.2.0 / C++17. Synthetic engineering drawings and rates.

| Check | Observed result |
| --- | --- |
| Native build | CLI, original core tests, CAD tests and OpenGL viewer compiled |
| Core regressions | 20 geometry, reporting, quoting, transform and picking checks passed |
| CAD algorithms | Concavity regression, invalid topology, winding invariance, inch conversion and DXF roundtrip passed |
| Differential oracle | Optimized hole-pair finding set matched brute force on 200 seeded 40-hole scenes |
| Material triangulation | Triangle area equals polygonal domain area; centroids remain in boundary and outside circular cutouts |
| Sparse 10000-hole benchmark | 49,995,000 possible pairs, 0 exact pair-distance evaluations, 4.1222 ms in the recorded run |
| CLI integration | Valid CAD=0, rule violation=2, unsupported/malformed CAD=1; JSON/SVG/normalized DXF inspected |
| Native graphics | Real OpenGL 3.3 Intel Iris Xe capture of imported notched fixture succeeded |

`docs/media/notched_fixture.png` is actual GPU framebuffer readback. `docs/evidence/cad_benchmark.json` is measured output. Sparse pair counts do not count interval comparisons; dense inputs may degrade quadratically. Topology/mesh tests are useful regressions, not an exact-predicate geometry proof.

CI builds with Linux GCC and Windows MSVC and captures a Mesa-rendered DXF fixture. The workflow attached to the published commit is the remote validation source.
