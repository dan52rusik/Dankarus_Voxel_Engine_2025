// Decompiled with JetBrains decompiler
// Type: WaterSimulationApplyFlows
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using Unity.Burst;
using Unity.Collections;
using Unity.Collections.LowLevel.Unsafe;
using Unity.Jobs;
using Unity.Mathematics;

#nullable disable
[BurstCompile(CompileSynchronously = true)]
public struct WaterSimulationApplyFlows : IJobParallelFor
{
  public NativeArray<ChunkKey> processingChunks;
  public NativeList<ChunkKey>.ParallelWriter nonFlowingChunks;
  public UnsafeParallelHashSet<ChunkKey>.ParallelWriter activeChunkSet;
  public UnsafeParallelHashMap<ChunkKey, WaterDataHandle> waterDataHandles;
  public NativeArray<WaterStats> waterStats;
  [PublicizedFrom(EAccessModifier.Private)]
  public WaterNeighborCacheNative neighborCache;
  [PublicizedFrom(EAccessModifier.Private)]
  public WaterStats stats;

  public void Execute(int chunkIndex)
  {
    this.neighborCache = WaterNeighborCacheNative.InitializeCache(this.waterDataHandles);
    ChunkKey processingChunk = this.processingChunks[chunkIndex];
    this.stats = this.waterStats[chunkIndex];
    WaterDataHandle waterDataHandle;
    if (this.waterDataHandles.TryGetValue(processingChunk, out waterDataHandle))
    {
      waterDataHandle.ApplyEnqueuedFlows();
      if (waterDataHandle.flowVoxels.IsEmpty)
      {
        this.nonFlowingChunks.AddNoResize(processingChunk);
      }
      else
      {
        this.neighborCache.SetChunk(processingChunk);
        UnsafeParallelHashMap<int, int>.Enumerator enumerator = waterDataHandle.flowVoxels.GetEnumerator();
        while (enumerator.MoveNext())
        {
          KeyValue<int, int> current = enumerator.Current;
          int key = current.Key;
          current = enumerator.Current;
          int x = current.Value;
          int3 voxelCoords = WaterDataHandle.GetVoxelCoords(key);
          int num1 = waterDataHandle.voxelData.Get(key);
          int num2;
          if (waterDataHandle.IsInGroundWater(voxelCoords.x, voxelCoords.y, voxelCoords.z))
            num2 = math.min(x, 19500);
          else if (x != 0)
            num2 = num1 + x;
          else
            continue;
          waterDataHandle.voxelData.Set(key, num2);
          waterDataHandle.SetVoxelActive(key);
          this.neighborCache.SetVoxel(voxelCoords.x, voxelCoords.y, voxelCoords.z);
          this.WakeNeighbor(WaterNeighborCacheNative.X_NEG);
          this.WakeNeighbor(WaterNeighborCacheNative.X_POS);
          this.WakeNeighbor(1);
          this.WakeNeighbor(-1);
          this.WakeNeighbor(WaterNeighborCacheNative.Z_NEG);
          this.WakeNeighbor(WaterNeighborCacheNative.Z_POS);
        }
        this.activeChunkSet.Add(processingChunk);
      }
    }
    this.waterStats[chunkIndex] = this.stats;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void WakeNeighbor(int _yOffset)
  {
    int _y = this.neighborCache.voxelY + _yOffset;
    if (_y < 0 || _y > (int) byte.MaxValue)
      return;
    this.neighborCache.center.SetVoxelActive(this.neighborCache.voxelX, _y, this.neighborCache.voxelZ);
    ++this.stats.NumVoxelsWokeUp;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void WakeNeighbor(int2 _xzOffset)
  {
    ChunkKey _chunkKey;
    WaterDataHandle _dataHandle;
    int _x;
    int _y;
    int _z;
    if (!this.neighborCache.TryGetNeighbor(_xzOffset, out _chunkKey, out _dataHandle, out _x, out _y, out _z))
      return;
    if (_chunkKey.Equals(this.neighborCache.chunkKey))
    {
      _dataHandle.SetVoxelActive(_x, _y, _z);
    }
    else
    {
      _dataHandle.EnqueueVoxelActive(_x, _y, _z);
      this.activeChunkSet.Add(_chunkKey);
    }
    ++this.stats.NumVoxelsWokeUp;
  }
}
