
# Install
`task conan:install`

- Taskfile
- Conan config
- CMake config
  - 📦 4. Package Configs & Export Targets
  - Depndency Management Internals
  - Presets
  -Toolchain files
- CI 

- Camke Details:
  - CPackConfig.cmake
  - Packaging-Konfig (generiert Installer/Archive)
  - CPackSourceConfig.cmake


## Arm64 crossbuild
conan install . -of build-arm64 \
  -pr:b=default \
  -pr:h=profiles/linux-arm64-gcc13 \
  -pr:h=local-toolchain \
  --build=missing

conan build . -bf build-arm64


# Pipeline
`libconfig` → `Harfbuzz` → `Freetype` → `OpenCL` →`libpng`


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
