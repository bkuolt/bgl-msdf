
# Calculate SDF
1) Find font with `libfontconfig`
2) Initialize `freetype` and load font face
3) Shape text with `Harfbuzz`→ glyph IDs and positions
4) Write glyphs in bitmap
5) Create `OpenCL` from it and upload it to the GPU
6) Run SDF `OpenCL`  kernel
7) Download texture
8) Save as PNG file

# Use SDF
1) Text vorbereiten (Runs bilden)
2) Run `Harfbuzz` for text -> glyph ids and positions
3) and look up in Glyph-Cache & SDF/MSDF Atlas
4) Upload `OpenCL` buffer
5) Run kernel -> for each glyph, render SDF
