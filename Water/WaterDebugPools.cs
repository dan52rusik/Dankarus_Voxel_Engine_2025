// Decompiled with JetBrains decompiler
// Type: WaterDebugPools
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

#nullable disable
public static class WaterDebugPools
{
  [PublicizedFrom(EAccessModifier.Private)]
  public const int maxActiveChunks = 250;
  [PublicizedFrom(EAccessModifier.Private)]
  public const int numLayers = 16 /*0x10*/;
  public static MemoryPooledObject<WaterDebugRenderer> rendererPool;
  public static MemoryPooledObject<WaterDebugRendererLayer> layerPool;

  public static void CreatePools()
  {
    WaterDebugPools.rendererPool = new MemoryPooledObject<WaterDebugRenderer>(250);
    WaterDebugPools.layerPool = new MemoryPooledObject<WaterDebugRendererLayer>(4000);
  }

  public static void Cleanup()
  {
    WaterDebugPools.rendererPool?.Cleanup();
    WaterDebugPools.rendererPool = (MemoryPooledObject<WaterDebugRenderer>) null;
    WaterDebugPools.layerPool?.Cleanup();
    WaterDebugPools.layerPool = (MemoryPooledObject<WaterDebugRendererLayer>) null;
  }
}
