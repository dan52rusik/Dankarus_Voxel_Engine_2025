// Decompiled with JetBrains decompiler
// Type: WaterDebugRenderer
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using UnityEngine;

#nullable disable
public class WaterDebugRenderer : IMemoryPoolableObject
{
  [PublicizedFrom(EAccessModifier.Private)]
  public const int numLayers = 16 /*0x10*/;
  [PublicizedFrom(EAccessModifier.Private)]
  public Vector3 chunkOrigin = Vector3.zero;
  [PublicizedFrom(EAccessModifier.Private)]
  public WaterDebugRendererLayer[] layers = new WaterDebugRendererLayer[16 /*0x10*/];
  [PublicizedFrom(EAccessModifier.Private)]
  public int[] activeLayers = new int[16 /*0x10*/];
  [PublicizedFrom(EAccessModifier.Private)]
  public int numActiveLayers;

  public void SetChunkOrigin(Vector3 _origin)
  {
    this.chunkOrigin = _origin;
    for (int index = 0; index < this.numActiveLayers; ++index)
    {
      int activeLayer = this.activeLayers[index];
      Vector3 _origin1 = this.chunkOrigin + Vector3.up * (float) activeLayer * 16f;
      this.layers[activeLayer].SetLayerOrigin(_origin1);
    }
  }

  public void SetWater(int _x, int _y, int _z, float mass)
  {
    int layerIndex = _y / 16 /*0x10*/;
    int _y1 = _y % 16 /*0x10*/;
    this.GetOrCreateLayer(layerIndex).SetWater(_x, _y1, _z, mass);
  }

  public void LoadFromChunk(Chunk chunk)
  {
    this.SetChunkOrigin((Vector3) chunk.GetWorldPos());
    for (int _x = 0; _x < 16 /*0x10*/; ++_x)
    {
      for (int _z = 0; _z < 16 /*0x10*/; ++_z)
      {
        for (int _y = 0; _y < 256 /*0x0100*/; ++_y)
        {
          float mass = (float) chunk.GetWater(_x, _y, _z).GetMass();
          if ((double) mass > 195.0)
            this.SetWater(_x, _y, _z, mass);
        }
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public WaterDebugRendererLayer GetOrCreateLayer(int layerIndex)
  {
    WaterDebugRendererLayer layer = this.layers[layerIndex];
    if (layer == null)
    {
      layer = WaterDebugPools.layerPool.AllocSync(false);
      Vector3 _origin = this.chunkOrigin + Vector3.up * (float) layerIndex * 16f;
      layer.SetLayerOrigin(_origin);
      this.layers[layerIndex] = layer;
      this.activeLayers[this.numActiveLayers] = layerIndex;
      ++this.numActiveLayers;
      Array.Sort<int>(this.activeLayers, 0, this.numActiveLayers);
    }
    return layer;
  }

  public void Draw()
  {
    for (int index = 0; index < this.numActiveLayers; ++index)
      this.layers[this.activeLayers[index]].Draw();
  }

  public void Clear()
  {
    for (int index = 0; index < this.numActiveLayers; ++index)
    {
      int activeLayer = this.activeLayers[index];
      WaterDebugRendererLayer layer = this.layers[activeLayer];
      WaterDebugPools.layerPool.FreeSync(layer);
      this.layers[activeLayer] = (WaterDebugRendererLayer) null;
      this.activeLayers[index] = 0;
    }
    this.numActiveLayers = 0;
  }

  public void Cleanup() => this.Clear();

  public void Reset() => this.Clear();
}
