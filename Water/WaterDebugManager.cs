// Decompiled with JetBrains decompiler
// Type: WaterDebugManager
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using UnityEngine;

#nullable disable
public class WaterDebugManager
{
  [CompilerGenerated]
  [PublicizedFrom(EAccessModifier.Private)]
  public bool \u003CRenderingEnabled\u003Ek__BackingField = true;
  [PublicizedFrom(EAccessModifier.Private)]
  public ConcurrentQueue<WaterDebugManager.InitializedRenderer> newRenderers = new ConcurrentQueue<WaterDebugManager.InitializedRenderer>();
  [PublicizedFrom(EAccessModifier.Private)]
  public Dictionary<long, WaterDebugRenderer> activeRenderers = new Dictionary<long, WaterDebugRenderer>();
  [PublicizedFrom(EAccessModifier.Private)]
  public ConcurrentQueue<long> renderersToRemove = new ConcurrentQueue<long>();

  public bool RenderingEnabled
  {
    get => this.\u003CRenderingEnabled\u003Ek__BackingField;
    set => this.\u003CRenderingEnabled\u003Ek__BackingField = value;
  }

  public void InitializeDebugRender(Chunk chunk)
  {
    WaterDebugRenderer waterDebugRenderer = WaterDebugPools.rendererPool.AllocSync(true);
    waterDebugRenderer.LoadFromChunk(chunk);
    chunk.AssignWaterDebugRenderer(new WaterDebugManager.RendererHandle(chunk, this));
    this.newRenderers.Enqueue(new WaterDebugManager.InitializedRenderer()
    {
      chunkKey = chunk.Key,
      renderer = waterDebugRenderer
    });
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void ReturnRenderer(long key) => this.renderersToRemove.Enqueue(key);

  [PublicizedFrom(EAccessModifier.Private)]
  public void UpdateRenderers()
  {
    WaterDebugManager.InitializedRenderer result1;
    while (this.newRenderers.TryDequeue(out result1))
    {
      WaterDebugRenderer _t;
      if (this.activeRenderers.TryGetValue(result1.chunkKey, out _t))
      {
        WaterDebugPools.rendererPool.FreeSync(_t);
        this.activeRenderers.Remove(result1.chunkKey);
      }
      this.activeRenderers.Add(result1.chunkKey, result1.renderer);
    }
    long result2;
    while (this.renderersToRemove.TryDequeue(out result2))
    {
      WaterDebugRenderer _t;
      if (this.activeRenderers.TryGetValue(result2, out _t))
      {
        WaterDebugPools.rendererPool.FreeSync(_t);
        this.activeRenderers.Remove(result2);
      }
    }
  }

  public void DebugDraw()
  {
    this.UpdateRenderers();
    if (!this.RenderingEnabled)
      return;
    foreach (WaterDebugRenderer waterDebugRenderer in this.activeRenderers.Values)
      waterDebugRenderer.Draw();
  }

  public void Cleanup()
  {
    WaterDebugManager.InitializedRenderer result;
    while (this.newRenderers.TryDequeue(out result))
      WaterDebugPools.rendererPool.FreeSync(result.renderer);
    foreach (WaterDebugRenderer _t in this.activeRenderers.Values)
      WaterDebugPools.rendererPool.FreeSync(_t);
    this.activeRenderers.Clear();
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public struct InitializedRenderer
  {
    public long chunkKey;
    public WaterDebugRenderer renderer;
  }

  public struct RendererHandle
  {
    [PublicizedFrom(EAccessModifier.Private)]
    public WaterDebugManager manager;
    [PublicizedFrom(EAccessModifier.Private)]
    public long? key;

    public bool IsValid => this.manager != null && this.key.HasValue;

    public RendererHandle(Chunk _chunk, WaterDebugManager _manager)
    {
      this.manager = _manager;
      this.key = new long?(_chunk.Key);
    }

    [Conditional("UNITY_EDITOR")]
    public void SetChunkOrigin(Vector3i _origin)
    {
      WaterDebugRenderer waterDebugRenderer;
      if (!this.IsValid || !this.manager.activeRenderers.TryGetValue(this.key.Value, out waterDebugRenderer))
        return;
      waterDebugRenderer.SetChunkOrigin((Vector3) _origin);
    }

    [Conditional("UNITY_EDITOR")]
    public void SetWater(int _x, int _y, int _z, float mass)
    {
      WaterDebugRenderer waterDebugRenderer;
      if (!this.IsValid || !this.manager.activeRenderers.TryGetValue(this.key.Value, out waterDebugRenderer))
        return;
      waterDebugRenderer.SetWater(_x, _y, _z, mass);
    }

    [Conditional("UNITY_EDITOR")]
    public void Reset()
    {
      if (this.IsValid)
        this.manager.ReturnRenderer(this.key.Value);
      this.manager = (WaterDebugManager) null;
      this.key = new long?();
    }
  }
}
