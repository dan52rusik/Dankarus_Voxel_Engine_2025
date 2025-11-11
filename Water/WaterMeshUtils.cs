// Decompiled with JetBrains decompiler
// Type: WaterMeshUtils
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using UnityEngine;

#nullable disable
public static class WaterMeshUtils
{
  public static void RenderFace(
    Vector3[] _vertices,
    LightingAround _lightingAround,
    long _textureFull,
    VoxelMesh[] _meshes,
    Vector2 UVdata,
    bool _alternateWinding = false)
  {
    _meshes[1].AddBasicQuad(_vertices, Color.white, UVdata, true, _alternateWinding);
  }
}
