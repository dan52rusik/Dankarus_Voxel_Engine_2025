// Decompiled with JetBrains decompiler
// Type: WaterSimulationPostProcess
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using Unity.Burst;
using Unity.Collections;
using Unity.Collections.LowLevel.Unsafe;
using Unity.Jobs;

#nullable disable
[BurstCompile(CompileSynchronously = true)]
public struct WaterSimulationPostProcess : IJob
{
  public NativeArray<ChunkKey> processingChunks;
  public NativeList<ChunkKey> nonFlowingChunks;
  public UnsafeParallelHashSet<ChunkKey> activeChunks;
  public UnsafeParallelHashMap<ChunkKey, WaterDataHandle> waterDataHandles;

  public void Execute()
  {
    for (int index = 0; index < this.nonFlowingChunks.Length; ++index)
    {
      if (this.waterDataHandles.TryGetValue(this.nonFlowingChunks[index], out WaterDataHandle _))
        this.activeChunks.Remove(this.nonFlowingChunks[index]);
    }
    for (int index = 0; index < this.processingChunks.Length; ++index)
    {
      ChunkKey processingChunk = this.processingChunks[index];
      WaterDataHandle waterDataHandle;
      if (this.waterDataHandles.TryGetValue(processingChunk, out waterDataHandle) && waterDataHandle.activationsFromOtherChunks.Count > 0)
      {
        waterDataHandle.ApplyEnqueuedActivations();
        this.activeChunks.Add(processingChunk);
      }
    }
  }
}
