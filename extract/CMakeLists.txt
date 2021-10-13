cmake_minimum_required(VERSION 3.12)

project(AssetExtract VERSION 1.0)

build_target(
  NAME
  extract
  TYPE
  ESMODULE
  SOURCES
  extract.cpp
  LINKS
  insomnia
  AUTHOR
  "Lukas Cone"
  DESCR
  "Insomnia Engine asset extract"
  START_YEAR
  2017)
