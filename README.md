# Insomnia

Insomnia is a collection of modding tools and research for Insomniacs 7th gen engine for PS3 platform

This toolset runs on Spike foundation.

Head to this **[Wiki](https://github.com/PredatorCZ/Spike/wiki/Spike)** for more information on how to effectively use it.
<h2>Module list</h2>
<ul>
<li><a href="#Extract-Assets">Extract Assets</a></li>
<li><a href="#Extract-Effect">Extract Effect</a></li>
<li><a href="#Extract-PSARC">Extract PSARC</a></li>
</ul>

## Extract Assets

### Module command: extract_assets

Extracts packed assets, converts textures.
In addition, converts mobys and ties into GLTF format.

### Input file patterns: `^assetlookup.dat$`

### Settings

- **convert-shaders**

  **CLI Long:** ***--convert-shaders***\
  **CLI Short:** ***-s***

  **Default value:** true

  Convert shaders into XML format.

- **extract-filter**

  **CLI Long:** ***--extract-filter***\
  **CLI Short:** ***-e***

  **Default value:** Mobys | Ties | Shrubs | Foliages | Zones | Textures | Shaders | Cinematics | Animsets | Cubemaps

  Select groups that should be extracted.

## Extract Effect

### Module command: extract_effect

Extracts and converts textures from `vfx_system_header.dat`.

## Extract PSARC

### Module command: extract_psarc

Extracts psarc archives.



## [Latest Release](https://github.com/PredatorCZ/InsomniaToolset/releases)

## License

This toolset is available under GPL v3 license. (See LICENSE)\
This toolset uses following libraries:

- Spike, Copyright (c) 2016-2024 Lukas Cone (Apache 2)
