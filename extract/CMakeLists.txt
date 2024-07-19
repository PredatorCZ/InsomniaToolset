cmake_minimum_required(VERSION 3.12)

project(AssetExtract)

build_target(
  NAME
  extract_assets
  TYPE
  ESMODULE
  VERSION
  1
  SOURCES
  extract.cpp
  gltf.cpp
  LINKS
  gltf-interface
  insomnia-interface
  AUTHOR
  "Lukas Cone"
  DESCR
  "Insomnia Engine asset extract"
  START_YEAR
  2017)
