# SkyrimSE Fear mod

A plugin for SkyrimSE that aims to increase difficulty by adding fear/tension mechanics.
Affects NPCs too, but player mostly.

## Motivations

The main purpose of this project is for me to have something to apply new things on as I learn C++.
The code itself works, but that's not the point, and as such this is more a learning project than an actual SkyrimSE mod. You shouldn't use this for an actual playthrough.

## Building

### Requirements

- git
- cmake
- Visual Studio 2022 with v143 C++ toolset (C++ Desktop Development package)

```
git clone https://github.com/K-Patsouris/FearSE/
cd FearSE
git submodule update --init --recursive
cmake --preset vs2022
```

You can edit `CMakePresets.json` to add new presets, if you don't have/want Visual Studio 2022.
It should work just fine with 2019 and v142 toolset, but I haven't tested anything older.
