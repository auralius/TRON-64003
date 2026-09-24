# H/M + Serial Upload update

This update moves the micro:bit μT-Kernel firmware from the legacy fixed CUTE v1 H5 8+24 topology to the current CUTE v2 configurable H/M folded-PW dual-DW/PW W32 family.

Key changes:

- CUTE v2 128-byte base header + dynamic 40-byte layer records.
- Variable H hybrid depth and M hidden-head depth.
- Sequential micro:bit execution of DWA→PWA and DWB→PWB branches with 16+16→32 concat.
- MLP split head support: M×(32→32), then 32→1 objectness and 32→4 box projection.
- A/B raw-flash slots remain 40 KiB each.
- Package validation now covers layer counts/shapes, CRCs, payload ranges/alignment, and exact INT8 quantization boundaries.
- New `MCAP` command allows Studio to preflight firmware capabilities before erase/write.
- New `MIN2` command lets Studio verify H/M/layer count/package bytes/label after activation.
- Legacy `MINF` remains available.
- Noodle initial arena is explicitly 48 KiB for the μT-Kernel detector build.

Host-side syntax checks were performed for the modified C/C++ sources. A full ARM cross-build and hardware run are still required on the development machine.
