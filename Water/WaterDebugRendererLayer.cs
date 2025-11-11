// Decompiled with JetBrains decompiler
// Type: WaterDebugRendererLayer
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using System.Runtime.CompilerServices;
using Unity.Mathematics;
using UnityEngine;

#nullable disable
public class WaterDebugRendererLayer : IMemoryPoolableObject
{
  public const int dimX = 16 /*0x10*/;
  public const int dimY = 16 /*0x10*/;
  public const int dimZ = 16 /*0x10*/;
  public const int elementsPerLayer = 4096 /*0x1000*/;
  [PublicizedFrom(EAccessModifier.Private)]
  public const float SCALE_CUTOFF = 0.01f;
  [PublicizedFrom(EAccessModifier.Private)]
  public const float RENDER_SCALE = 0.9f;
  [PublicizedFrom(EAccessModifier.Private)]
  public static readonly float4 waterColor = new float4(0.0f, 0.0f, 1f, 1f);
  [PublicizedFrom(EAccessModifier.Private)]
  public static readonly float4 overfullColor = new float4(1f, 0.0f, 1f, 1f);
  [PublicizedFrom(EAccessModifier.Private)]
  public MaterialPropertyBlock materialProperties;
  [PublicizedFrom(EAccessModifier.Private)]
  public Vector3 layerOrigin;
  [PublicizedFrom(EAccessModifier.Private)]
  public Bounds bounds;
  [PublicizedFrom(EAccessModifier.Private)]
  public Matrix4x4[] transforms;
  [PublicizedFrom(EAccessModifier.Private)]
  public float4[] colors;
  [PublicizedFrom(EAccessModifier.Private)]
  public bool transformsHaveChanged;
  [PublicizedFrom(EAccessModifier.Private)]
  public float[] normalizedMass;
  [PublicizedFrom(EAccessModifier.Private)]
  public bool massesHaveChanged;
  [PublicizedFrom(EAccessModifier.Private)]
  public ComputeBuffer transformsBuffer;
  [PublicizedFrom(EAccessModifier.Private)]
  public ComputeBuffer massBuffer;
  [PublicizedFrom(EAccessModifier.Private)]
  public ComputeBuffer colorBuffer;
  [CompilerGenerated]
  [PublicizedFrom(EAccessModifier.Private)]
  public bool \u003CIsInitialized\u003Ek__BackingField;
  [PublicizedFrom(EAccessModifier.Private)]
  public int totalWater;

  public bool IsInitialized
  {
    get => this.\u003CIsInitialized\u003Ek__BackingField;
    [PublicizedFrom(EAccessModifier.Private)] set
    {
      this.\u003CIsInitialized\u003Ek__BackingField = value;
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void InitializeData()
  {
    this.transforms = new Matrix4x4[4096 /*0x1000*/];
    this.colors = new float4[4096 /*0x1000*/];
    this.normalizedMass = new float[4096 /*0x1000*/];
    this.RegenerateTransforms();
    Origin.OriginChanged += new Action<Vector3>(this.OnOriginChanged);
    this.IsInitialized = true;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void RegenerateTransforms()
  {
    Vector3 s = Vector3.one * 0.9f;
    Vector3 vector3_1 = (Vector3.one - s) * 0.5f;
    for (int index1 = 0; index1 < 16 /*0x10*/; ++index1)
    {
      for (int index2 = 0; index2 < 16 /*0x10*/; ++index2)
      {
        for (int index3 = 0; index3 < 16 /*0x10*/; ++index3)
          this.transforms[this.CoordToIndex(index1, index2, index3)] = Matrix4x4.TRS(this.layerOrigin + new Vector3((float) index1, (float) index2, (float) index3) - Origin.position + vector3_1 + Vector3.one * 0.5f, Quaternion.identity, s);
      }
    }
    Vector3 vector3_2 = this.layerOrigin - Origin.position;
    Vector3 vector3_3 = this.layerOrigin - Origin.position + new Vector3(16f, 16f, 16f);
    this.bounds = new Bounds((vector3_3 + vector3_2) / 2f, vector3_3 - vector3_2);
    this.transformsHaveChanged = true;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void OnOriginChanged(Vector3 _origin)
  {
    if (!this.IsInitialized)
      return;
    this.RegenerateTransforms();
  }

  public void SetLayerOrigin(Vector3 _origin)
  {
    this.layerOrigin = _origin;
    if (!this.IsInitialized)
      return;
    this.RegenerateTransforms();
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public int CoordToIndex(int _x, int _y, int _z) => _x + 16 /*0x10*/ * _y + 256 /*0x0100*/ * _z;

  public void SetWater(int _x, int _y, int _z, float mass)
  {
    float num = mass / 19500f;
    if (!this.IsInitialized)
    {
      if ((double) num < 0.0099999997764825821)
        return;
      this.InitializeData();
    }
    int index = this.CoordToIndex(_x, _y, _z);
    if ((double) this.normalizedMass[index] < 0.0099999997764825821 && (double) num > 0.0099999997764825821)
      ++this.totalWater;
    else if ((double) this.normalizedMass[index] > 0.0099999997764825821 && (double) num < 0.0099999997764825821)
      --this.totalWater;
    this.normalizedMass[index] = num;
    float s = math.max((float) (((double) mass - 19500.0) / (double) ushort.MaxValue), 0.0f);
    this.colors[index] = math.lerp(WaterDebugRendererLayer.waterColor, WaterDebugRendererLayer.overfullColor, s);
    this.massesHaveChanged = true;
  }

  public void Draw()
  {
    if (this.totalWater == 0)
      return;
    if (this.materialProperties == null)
    {
      this.materialProperties = new MaterialPropertyBlock();
      this.materialProperties.SetFloat("_ScaleCutoff", 0.01f);
    }
    if (this.transformsBuffer == null)
    {
      this.transformsBuffer = new ComputeBuffer(4096 /*0x1000*/, 64 /*0x40*/);
      this.materialProperties.SetBuffer("_Transforms", this.transformsBuffer);
    }
    if (this.colorBuffer == null)
    {
      this.colorBuffer = new ComputeBuffer(4096 /*0x1000*/, 16 /*0x10*/);
      this.materialProperties.SetBuffer("_Colors", this.colorBuffer);
    }
    if (this.transformsHaveChanged)
    {
      this.transformsBuffer.SetData((Array) this.transforms);
      this.transformsHaveChanged = false;
    }
    if (this.massBuffer == null)
    {
      this.massBuffer = new ComputeBuffer(4096 /*0x1000*/, 4);
      this.materialProperties.SetBuffer("_Scales", this.massBuffer);
    }
    if (this.massesHaveChanged)
    {
      this.massBuffer.SetData((Array) this.normalizedMass);
      this.colorBuffer.SetData((Array) this.colors);
      this.massesHaveChanged = false;
    }
    Graphics.DrawMeshInstancedProcedural(WaterDebugAssets.CubeMesh, 0, WaterDebugAssets.DebugMaterial, this.bounds, 4096 /*0x1000*/, this.materialProperties);
  }

  public void Reset()
  {
    if (this.IsInitialized)
    {
      Origin.OriginChanged -= new Action<Vector3>(this.OnOriginChanged);
      this.transforms = (Matrix4x4[]) null;
      this.normalizedMass = (float[]) null;
      this.IsInitialized = false;
    }
    this.totalWater = 0;
    this.transformsHaveChanged = false;
    this.massesHaveChanged = false;
  }

  public void Cleanup()
  {
    if (this.transformsBuffer != null)
    {
      this.transformsBuffer.Dispose();
      this.transformsBuffer = (ComputeBuffer) null;
    }
    if (this.massBuffer != null)
    {
      this.massBuffer.Dispose();
      this.massBuffer = (ComputeBuffer) null;
    }
    if (this.colorBuffer == null)
      return;
    this.colorBuffer.Dispose();
    this.colorBuffer = (ComputeBuffer) null;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  static WaterDebugRendererLayer()
  {
  }
}
