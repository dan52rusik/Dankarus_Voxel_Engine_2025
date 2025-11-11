// Decompiled with JetBrains decompiler
// Type: WaterSimulationNative
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Text;
using Unity.Collections;
using Unity.Collections.LowLevel.Unsafe;
using Unity.Jobs;
using Unity.Jobs.LowLevel.Unsafe;
using UnityEngine;

#nullable disable
public class WaterSimulationNative
{
  [CompilerGenerated]
  [PublicizedFrom(EAccessModifier.Private)]
  public static WaterSimulationNative \u003CInstance\u003Ek__BackingField = new WaterSimulationNative();
  public bool ShouldEnable = true;
  [CompilerGenerated]
  [PublicizedFrom(EAccessModifier.Private)]
  public bool \u003CIsInitialized\u003Ek__BackingField;
  [PublicizedFrom(EAccessModifier.Private)]
  public bool isPaused;
  [PublicizedFrom(EAccessModifier.Private)]
  public Dictionary<ChunkKey, NativeSafeHandle<WaterDataHandle>> usedHandles = new Dictionary<ChunkKey, NativeSafeHandle<WaterDataHandle>>();
  [PublicizedFrom(EAccessModifier.Private)]
  public ConcurrentQueue<NativeSafeHandle<WaterDataHandle>> freeHandles = new ConcurrentQueue<NativeSafeHandle<WaterDataHandle>>();
  [PublicizedFrom(EAccessModifier.Private)]
  public ConcurrentQueue<WaterSimulationNative.HandleInitRequest> newInitializedHandles = new ConcurrentQueue<WaterSimulationNative.HandleInitRequest>();
  [PublicizedFrom(EAccessModifier.Private)]
  public ConcurrentQueue<ChunkKey> handlesToRemove = new ConcurrentQueue<ChunkKey>();
  [PublicizedFrom(EAccessModifier.Private)]
  public UnsafeParallelHashSet<ChunkKey> activeHandles;
  [PublicizedFrom(EAccessModifier.Private)]
  public UnsafeParallelHashMap<ChunkKey, WaterDataHandle> waterDataHandles;
  [PublicizedFrom(EAccessModifier.Private)]
  public UnsafeParallelHashSet<ChunkKey> modifiedChunks;
  [PublicizedFrom(EAccessModifier.Private)]
  public GroundWaterHeightMap groundWaterHeightMap;
  public WaterSimulationApplyChanges changeApplier;

  public static WaterSimulationNative Instance
  {
    get => WaterSimulationNative.\u003CInstance\u003Ek__BackingField;
    [PublicizedFrom(EAccessModifier.Private)] set
    {
      WaterSimulationNative.\u003CInstance\u003Ek__BackingField = value;
    }
  }

  public bool IsInitialized
  {
    get => this.\u003CIsInitialized\u003Ek__BackingField;
    [PublicizedFrom(EAccessModifier.Private)] set
    {
      this.\u003CIsInitialized\u003Ek__BackingField = value;
    }
  }

  public bool IsPaused => this.isPaused;

  public void Init(ChunkCluster _cc)
  {
    this.changeApplier = new WaterSimulationApplyChanges(_cc);
    if (!this.ShouldEnable || !SingletonMonoBehaviour<ConnectionManager>.Instance.IsServer)
      return;
    if (this.waterDataHandles.IsCreated || this.modifiedChunks.IsCreated)
      Debug.LogError((object) "Last water simulation data was disposed of and may have leaked");
    this.activeHandles = new UnsafeParallelHashSet<ChunkKey>(500, AllocatorManager.Persistent);
    this.waterDataHandles = new UnsafeParallelHashMap<ChunkKey, WaterDataHandle>(500, AllocatorManager.Persistent);
    this.modifiedChunks = new UnsafeParallelHashSet<ChunkKey>(500, AllocatorManager.Persistent);
    this.groundWaterHeightMap = new GroundWaterHeightMap(GameManager.Instance.World);
    this.IsInitialized = true;
    this.isPaused = false;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public bool IsChunkInWorldBounds(Chunk _c)
  {
    Vector3i _minSize;
    Vector3i _maxSize;
    GameManager.Instance.World.GetWorldExtent(out _minSize, out _maxSize);
    return _c.X * 16 /*0x10*/ >= _minSize.x && _c.X * 16 /*0x10*/ < _maxSize.x && _c.Z * 16 /*0x10*/ >= _minSize.z && _c.Z * 16 /*0x10*/ < _maxSize.z;
  }

  public void InitializeChunk(Chunk _c)
  {
    if (!this.IsInitialized || !this.IsChunkInWorldBounds(_c))
      return;
    NativeSafeHandle<WaterDataHandle> result;
    WaterDataHandle _target;
    if (this.freeHandles.TryDequeue(out result))
    {
      _target = result.Target;
      _target.Clear();
    }
    else
    {
      _target = WaterDataHandle.AllocateNew(Allocator.Persistent);
      result = new NativeSafeHandle<WaterDataHandle>(ref _target, Allocator.Persistent);
    }
    _target.InitializeFromChunk(_c, this.groundWaterHeightMap);
    this.newInitializedHandles.Enqueue(new WaterSimulationNative.HandleInitRequest()
    {
      chunkKey = new ChunkKey((IChunk) _c),
      safeHandle = result
    });
    _c.AssignWaterSimHandle(new WaterSimulationNative.ChunkHandle(this, _c));
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void CopyInitializedChunksToNative()
  {
    WaterSimulationNative.HandleInitRequest result;
    while (this.newInitializedHandles.TryDequeue(out result))
    {
      ChunkKey chunkKey = result.chunkKey;
      NativeSafeHandle<WaterDataHandle> safeHandle = result.safeHandle;
      NativeSafeHandle<WaterDataHandle> nativeSafeHandle;
      if (this.usedHandles.TryGetValue(chunkKey, out nativeSafeHandle))
      {
        this.freeHandles.Enqueue(nativeSafeHandle);
        this.usedHandles[chunkKey] = safeHandle;
      }
      else
        this.usedHandles.Add(chunkKey, safeHandle);
      WaterDataHandle target = safeHandle.Target;
      this.waterDataHandles[chunkKey] = target;
      if (target.HasActiveWater)
        this.activeHandles.Add(chunkKey);
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void ProcessPendingRemoves()
  {
    ChunkKey result;
    while (this.handlesToRemove.TryDequeue(out result))
    {
      NativeSafeHandle<WaterDataHandle> nativeSafeHandle;
      if (this.usedHandles.TryGetValue(result, out nativeSafeHandle))
      {
        this.usedHandles.Remove(result);
        this.freeHandles.Enqueue(nativeSafeHandle);
      }
      this.waterDataHandles.Remove(result);
      this.activeHandles.Remove(result);
    }
  }

  public void SetPaused(bool _isPaused) => this.isPaused = _isPaused;

  public void Step()
  {
    if (!this.isPaused)
    {
      this.SetPaused(true);
    }
    else
    {
      this.isPaused = false;
      this.Update();
      this.isPaused = true;
    }
  }

  public void Update()
  {
    if (!this.IsInitialized)
      return;
    this.ProcessPendingRemoves();
    this.CopyInitializedChunksToNative();
    if (this.isPaused || this.changeApplier.HasNetWorkLimitBeenReached())
      return;
    WaterStats stats = new WaterStats();
    if (!this.modifiedChunks.IsEmpty || !this.activeHandles.IsEmpty)
    {
      new WaterSimulationPreProcess()
      {
        activeChunks = this.activeHandles,
        waterDataHandles = this.waterDataHandles,
        modifiedChunks = this.modifiedChunks
      }.Run<WaterSimulationPreProcess>();
      if (!this.activeHandles.IsEmpty)
      {
        NativeArray<ChunkKey> nativeArray = this.activeHandles.ToNativeArray((AllocatorManager.AllocatorHandle) Allocator.TempJob);
        NativeList<ChunkKey> nativeList = new NativeList<ChunkKey>(nativeArray.Length, AllocatorManager.TempJob);
        NativeArray<WaterStats> array = new NativeArray<WaterStats>(nativeArray.Length, Allocator.TempJob);
        WaterSimulationCalcFlows jobData1 = new WaterSimulationCalcFlows()
        {
          processingChunks = nativeArray,
          waterStats = array,
          waterDataHandles = this.waterDataHandles
        };
        WaterSimulationApplyFlows jobData2 = new WaterSimulationApplyFlows()
        {
          processingChunks = nativeArray,
          nonFlowingChunks = nativeList.AsParallelWriter(),
          waterStats = array,
          waterDataHandles = this.waterDataHandles,
          activeChunkSet = this.activeHandles.AsParallelWriter()
        };
        WaterSimulationPostProcess jobData3 = new WaterSimulationPostProcess()
        {
          processingChunks = nativeArray,
          nonFlowingChunks = nativeList,
          activeChunks = this.activeHandles,
          waterDataHandles = this.waterDataHandles
        };
        int innerloopBatchCount = nativeArray.Length / JobsUtility.JobWorkerCount + 1;
        JobHandle dependsOn1 = jobData1.Schedule<WaterSimulationCalcFlows>(nativeArray.Length, innerloopBatchCount);
        JobHandle dependsOn2 = jobData2.Schedule<WaterSimulationApplyFlows>(nativeArray.Length, innerloopBatchCount, dependsOn1);
        JobHandle jobHandle = jobData3.Schedule<WaterSimulationPostProcess>(dependsOn2);
        JobHandle.ScheduleBatchedJobs();
        jobHandle.Complete();
        stats += WaterStats.Sum(array);
        array.Dispose();
        nativeArray.Dispose();
        nativeList.Dispose();
      }
    }
    this.ProcessPendingRemoves();
    UnsafeParallelHashMap<ChunkKey, WaterDataHandle>.Enumerator enumerator = this.waterDataHandles.GetEnumerator();
    while (enumerator.MoveNext())
    {
      KeyValue<ChunkKey, WaterDataHandle> current = enumerator.Current;
      ChunkKey key1 = current.Key;
      current = enumerator.Current;
      WaterDataHandle waterDataHandle = current.Value;
      if (waterDataHandle.HasFlows)
      {
        using (WaterSimulationApplyChanges.ChangesForChunk.Writer changeWriter = this.changeApplier.GetChangeWriter(WorldChunkCache.MakeChunkKey(key1.x, key1.z)))
        {
          UnsafeParallelHashMap<int, int>.Enumerator flowVoxels = waterDataHandle.FlowVoxels;
          while (flowVoxels.MoveNext())
          {
            int key2 = flowVoxels.Current.Key;
            WaterValue _waterValue = new WaterValue(waterDataHandle.voxelData.Get(key2));
            changeWriter.RecordChange(key2, _waterValue);
          }
          waterDataHandle.flowVoxels.Clear();
        }
      }
    }
    WaterStatsProfiler.SampleTick(stats);
  }

  public void Clear()
  {
    if (!this.IsInitialized)
      return;
    foreach (NativeSafeHandle<WaterDataHandle> nativeSafeHandle in this.usedHandles.Values)
      this.freeHandles.Enqueue(nativeSafeHandle);
    this.usedHandles.Clear();
    this.waterDataHandles.Clear();
    WaterSimulationNative.HandleInitRequest result;
    while (this.newInitializedHandles.TryDequeue(out result))
      this.freeHandles.Enqueue(result.safeHandle);
    this.handlesToRemove = new ConcurrentQueue<ChunkKey>();
    this.activeHandles.Clear();
    this.modifiedChunks.Clear();
  }

  public void Cleanup()
  {
    this.changeApplier?.Cleanup();
    this.changeApplier = (WaterSimulationApplyChanges) null;
    this.groundWaterHeightMap = (GroundWaterHeightMap) null;
    if (!this.IsInitialized)
      return;
    this.Clear();
    NativeSafeHandle<WaterDataHandle> result;
    while (this.freeHandles.TryDequeue(out result))
      result.Dispose();
    foreach (NativeSafeHandle<WaterDataHandle> nativeSafeHandle in this.usedHandles.Values)
      nativeSafeHandle.Dispose();
    this.usedHandles.Clear();
    if (this.activeHandles.IsCreated)
      this.activeHandles.Dispose();
    if (this.waterDataHandles.IsCreated)
      this.waterDataHandles.Dispose();
    if (this.modifiedChunks.IsCreated)
      this.modifiedChunks.Dispose();
    this.IsInitialized = false;
  }

  public string GetMemoryStats()
  {
    int count1 = this.usedHandles.Count;
    int count2 = this.freeHandles.Count;
    int count3 = this.newInitializedHandles.Count;
    int num1 = 0;
    foreach (NativeSafeHandle<WaterDataHandle> nativeSafeHandle in this.usedHandles.Values)
      num1 += nativeSafeHandle.Target.CalculateOwnedBytes();
    foreach (NativeSafeHandle<WaterDataHandle> freeHandle in this.freeHandles)
      num1 += freeHandle.Target.CalculateOwnedBytes();
    foreach (WaterSimulationNative.HandleInitRequest initializedHandle in this.newInitializedHandles)
      num1 += initializedHandle.safeHandle.Target.CalculateOwnedBytes();
    int num2 = ProfilerUtils.CalculateUnsafeParallelHashSetBytes<ChunkKey>(this.activeHandles) + ProfilerUtils.CalculateUnsafeParallelHashMapBytes<ChunkKey, WaterDataHandle>(this.waterDataHandles);
    return $"Allocated Handles: {count1 + count2 + count3}, Used Handles: {count1}, Free Handles: {count2}, Pending Handles: {count3}, Handle Contents (MB): {(double) num1 * 9.5367431640625E-07:F2}, Other Memory (MB): {(double) num2 * 9.5367431640625E-07:F2}, Total Memory (MB): {(double) (num1 + num2) * 9.5367431640625E-07:F2}";
  }

  public string GetMemoryStatsDetailed()
  {
    StringBuilder stringBuilder = new StringBuilder();
    stringBuilder.AppendLine("Used Handles:");
    foreach (KeyValuePair<ChunkKey, NativeSafeHandle<WaterDataHandle>> usedHandle in this.usedHandles)
      stringBuilder.AppendFormat("Chunk ({0},{1}): {2}\n", (object) usedHandle.Key.x, (object) usedHandle.Key.z, (object) usedHandle.Value.Target.GetMemoryStats());
    return stringBuilder.ToString();
  }

  [PublicizedFrom(EAccessModifier.Private)]
  static WaterSimulationNative()
  {
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public struct HandleInitRequest
  {
    public ChunkKey chunkKey;
    public NativeSafeHandle<WaterDataHandle> safeHandle;
  }

  public struct ChunkHandle(WaterSimulationNative _sim, Chunk _chunk)
  {
    [PublicizedFrom(EAccessModifier.Private)]
    public WaterSimulationNative sim = _sim;
    [PublicizedFrom(EAccessModifier.Private)]
    public ChunkKey chunkKey = new ChunkKey((IChunk) _chunk);

    public bool IsValid => this.sim != null;

    public void SetWaterMass(int _x, int _y, int _z, int _mass)
    {
      WaterDataHandle waterDataHandle;
      if (!this.IsValid || !this.sim.waterDataHandles.TryGetValue(this.chunkKey, out waterDataHandle))
        return;
      int voxelIndex = WaterDataHandle.GetVoxelIndex(_x, _y, _z);
      waterDataHandle.SetVoxelMass(voxelIndex, _mass);
      this.sim.activeHandles.Add(this.chunkKey);
    }

    public void SetVoxelSolid(int _x, int _y, int _z, BlockFaceFlag _flags)
    {
      WaterDataHandle waterDataHandle;
      if (!this.IsValid || !this.sim.waterDataHandles.TryGetValue(this.chunkKey, out waterDataHandle))
        return;
      waterDataHandle.SetVoxelSolid(_x, _y, _z, _flags);
      if (_flags == BlockFaceFlag.All)
        return;
      this.WakeNeighbours(_x, _y, _z);
    }

    public void WakeNeighbours(int _x, int _y, int _z)
    {
      if (!this.IsValid)
        return;
      WaterDataHandle waterDataHandle;
      if (this.sim.waterDataHandles.TryGetValue(this.chunkKey, out waterDataHandle))
        waterDataHandle.EnqueueVoxelWakeup(_x, _y, _z);
      this.sim.modifiedChunks.Add(this.chunkKey);
    }

    public void Reset()
    {
      if (this.IsValid && this.sim.IsInitialized)
        this.sim.handlesToRemove.Enqueue(this.chunkKey);
      this.sim = (WaterSimulationNative) null;
      this.chunkKey = new ChunkKey();
    }
  }
}
