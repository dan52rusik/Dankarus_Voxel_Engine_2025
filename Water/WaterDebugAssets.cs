// Decompiled with JetBrains decompiler
// Type: WaterDebugAssets
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using UnityEngine;

#nullable disable
public static class WaterDebugAssets
{
  [PublicizedFrom(EAccessModifier.Private)]
  public static Lazy<Mesh> cubeMesh = new Lazy<Mesh>(new Func<Mesh>(WaterDebugAssets.GenerateCubeMesh));
  [PublicizedFrom(EAccessModifier.Private)]
  public static Lazy<Material> sharedMaterial = new Lazy<Material>(new Func<Material>(WaterDebugAssets.CreateDebugMaterial));

  public static Mesh CubeMesh => WaterDebugAssets.cubeMesh.Value;

  public static Mesh GenerateCubeMesh()
  {
    Mesh cubeMesh = new Mesh();
    cubeMesh.vertices = new Vector3[8]
    {
      new Vector3(-0.5f, -0.5f, -0.5f),
      new Vector3(0.5f, -0.5f, -0.5f),
      new Vector3(0.5f, 0.5f, -0.5f),
      new Vector3(-0.5f, 0.5f, -0.5f),
      new Vector3(-0.5f, 0.5f, 0.5f),
      new Vector3(0.5f, 0.5f, 0.5f),
      new Vector3(0.5f, -0.5f, 0.5f),
      new Vector3(-0.5f, -0.5f, 0.5f)
    };
    cubeMesh.triangles = new int[36]
    {
      0,
      2,
      1,
      0,
      3,
      2,
      2,
      3,
      4,
      2,
      4,
      5,
      1,
      2,
      5,
      1,
      5,
      6,
      0,
      7,
      4,
      0,
      4,
      3,
      5,
      4,
      7,
      5,
      7,
      6,
      0,
      6,
      7,
      0,
      1,
      6
    };
    cubeMesh.Optimize();
    cubeMesh.RecalculateNormals();
    return cubeMesh;
  }

  public static Material DebugMaterial => WaterDebugAssets.sharedMaterial.Value;

  public static Material CreateDebugMaterial()
  {
    return new Material(Shader.Find("Debug/DebugInstancedProcedural"))
    {
      enableInstancing = true
    };
  }

  [PublicizedFrom(EAccessModifier.Private)]
  static WaterDebugAssets()
  {
  }
}
