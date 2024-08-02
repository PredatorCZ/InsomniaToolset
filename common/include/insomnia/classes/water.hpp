
struct WaterIndex {
  SVector face;
  int16 adjIndices[5];
};

struct WaterMesh {
  float unk[2];
  uint32 numVertices;
  uint32 numIndices;
  Vector4A16 vertices[0x190];
  WaterIndex indices[0x31B];
};

struct WaterMeshes : CoreClass {
  static constexpr uint32 ID = 0x25600;

  Vector4A16 unk0;
  uint32 numMeshes;
  WaterMesh meshes[];
};

void FByteswapper(WaterMesh &item) {
  FByteswapper(item.unk);
  FByteswapper(item.numVertices);
  FByteswapper(item.numIndices);
  FArraySwapper(item.vertices);
  FArraySwapper<uint16>(item.indices);
}

void FByteswapper(WaterMeshes &item) {
  FByteswapper(item.unk0);
  FByteswapper(item.numMeshes);

  for (uint32 i = 0; i < item.numMeshes; i++) {
    FByteswapper(item.meshes[i]);
  }
}
