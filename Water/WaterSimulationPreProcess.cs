// Decompiled with JetBrains decompiler
// Type: WaterSimulationPreProcess
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using Unity.Burst;
using Unity.Collections;
using Unity.Collections.LowLevel.Unsafe;
using Unity.Jobs;
using Unity.Mathematics;
using UnityEngine;

#nullable disable
[BurstCompile(CompileSynchronously = true)]
public struct WaterSimulationPreProcess : IJob
{
  public UnsafeParallelHashSet<ChunkKey> activeChunks;
  public UnsafeParallelHashMap<ChunkKey, WaterDataHandle> waterDataHandles;
  public UnsafeParallelHashSet<ChunkKey> modifiedChunks;
  [PublicizedFrom(EAccessModifier.Private)]
  public WaterNeighborCacheNative neighborCache;

  public void Execute()
  {
    this.neighborCache = WaterNeighborCacheNative.InitializeCache(this.waterDataHandles);
    using (UnsafeParallelHashSet<ChunkKey>.Enumerator enumerator1 = this.modifiedChunks.GetEnumerator())
    {
      while (enumerator1.MoveNext())
      {
        ChunkKey current = enumerator1.Current;
        WaterDataHandle waterDataHandle;
        if (this.waterDataHandles.TryGetValue(current, out waterDataHandle))
        {
          this.neighborCache.SetChunk(current);
          int num = 0;
          using (UnsafeParallelHashSet<int>.Enumerator enumerator2 = waterDataHandle.voxelsToWakeup.GetEnumerator())
          {
            while (enumerator2.MoveNext())
            {
              if (++num > 65536 /*0x010000*/)
              {
                Debug.LogError((object) $"[WaterSimulationPreProcess] Number of wakeups for chunk ({current.x}, {current.z}) has exceeded the volume of a chunk {65536 /*0x010000*/}.");
                break;
              }
              int3 voxelCoords = WaterDataHandle.GetVoxelCoords(enumerator2.Current);
              waterDataHandle.SetVoxelActive(voxelCoords.x, voxelCoords.y, voxelCoords.z);
              this.neighborCache.SetVoxel(voxelCoords.x, voxelCoords.y, voxelCoords.z);
              this.WakeNeighbor(WaterNeighborCacheNative.X_NEG);
              this.WakeNeighbor(WaterNeighborCacheNative.X_POS);
              this.WakeNeighbor(1);
              this.WakeNeighbor(-1);
              this.WakeNeighbor(WaterNeighborCacheNative.Z_NEG);
              this.WakeNeighbor(WaterNeighborCacheNative.Z_POS);
            }
            waterDataHandle.voxelsToWakeup.Clear();
            this.activeChunks.Add(current);
          }
        }
      }
      this.modifiedChunks.Clear();
      using (NativeArray<ChunkKey> nativeArray = this.activeChunks.ToNativeArray((AllocatorManager.AllocatorHandle) Allocator.Temp))
      {
        for (int index = 0; index < nativeArray.Length; ++index)
        {
          ChunkKey chunkKey = nativeArray[index];
          this.TryTrackChunk(chunkKey.x + 1, chunkKey.z);
          this.TryTrackChunk(chunkKey.x - 1, chunkKey.z);
          this.TryTrackChunk(chunkKey.x, chunkKey.z + 1);
          this.TryTrackChunk(chunkKey.x, chunkKey.z - 1);
        }
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void WakeNeighbor(int _yOffset)
  {
    int _y = this.neighborCache.voxelY + _yOffset;
    if (_y < 0 || _y > (int) byte.MaxValue)
      return;
    this.neighborCache.center.SetVoxelActive(this.neighborCache.voxelX, _y, this.neighborCache.voxelZ);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void WakeNeighbor(int2 _xzOffset)
  {
    WaterDataHandle _dataHandle;
    int _x;
    int _y;
    int _z;
    if (!this.neighborCache.TryGetNeighbor(_xzOffset, out ChunkKey _, out _dataHandle, out _x, out _y, out _z))
      return;
    _dataHandle.SetVoxelActive(_x, _y, _z);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void TryTrackChunk(int _chunkX, int _chunkZ)
  {
    ChunkKey key = new ChunkKey(_chunkX, _chunkZ);
    if (!this.waterDataHandles.ContainsKey(key))
      return;
    this.activeChunks.Add(key);
  }
}
