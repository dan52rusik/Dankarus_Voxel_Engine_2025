// Decompiled with JetBrains decompiler
// Type: WaterSimulationCalcFlows
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
public struct WaterSimulationCalcFlows : IJobParallelFor
{
  public NativeArray<ChunkKey> processingChunks;
  public NativeArray<WaterStats> waterStats;
  public UnsafeParallelHashMap<ChunkKey, WaterDataHandle> waterDataHandles;
  [PublicizedFrom(EAccessModifier.Private)]
  public WaterStats stats;
  [PublicizedFrom(EAccessModifier.Private)]
  public WaterNeighborCacheNative neighborCache;

  public void Execute(int chunkIndex)
  {
    ChunkKey processingChunk = this.processingChunks[chunkIndex];
    this.stats = this.waterStats[chunkIndex];
    this.neighborCache = WaterNeighborCacheNative.InitializeCache(this.waterDataHandles);
    this.ProcessFlows(processingChunk);
    this.waterStats[chunkIndex] = this.stats;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void ProcessFlows(ChunkKey chunkKey)
  {
    WaterDataHandle _chunkData;
    if (!this.waterDataHandles.TryGetValue(chunkKey, out _chunkData))
      return;
    ++this.stats.NumChunksProcessed;
    if (!_chunkData.HasActiveWater)
      return;
    ++this.stats.NumChunksActive;
    this.neighborCache.SetChunk(chunkKey);
    UnsafeBitArraySetIndicesEnumerator activeVoxelIndices = _chunkData.ActiveVoxelIndices;
    while (activeVoxelIndices.MoveNext())
    {
      ++this.stats.NumVoxelsProcessed;
      int current = activeVoxelIndices.Current;
      int3 voxelCoords = WaterDataHandle.GetVoxelCoords(current);
      int _mass1 = _chunkData.voxelData.Get(current);
      this.neighborCache.SetVoxel(voxelCoords.x, voxelCoords.y, voxelCoords.z);
      if (_chunkData.IsInGroundWater(voxelCoords.x, voxelCoords.y, voxelCoords.z))
      {
        WaterVoxelState _fromVoxelState = _chunkData.voxelState.Get(voxelCoords.x, voxelCoords.y, voxelCoords.z);
        if (_fromVoxelState.IsSolid())
        {
          _chunkData.SetVoxelInactive(current);
          ++this.stats.NumVoxelsPutToSleep;
        }
        else if (_mass1 != 19500)
          _chunkData.ApplyFlow(current, 19500);
        else if (_mass1 > 195 && this.ProcessGroundWaterFlowSide(chunkKey, _fromVoxelState, _mass1, WaterNeighborCacheNative.X_NEG) + this.ProcessGroundWaterFlowSide(chunkKey, _fromVoxelState, _mass1, WaterNeighborCacheNative.X_POS) + this.ProcessGroundWaterFlowSide(chunkKey, _fromVoxelState, _mass1, WaterNeighborCacheNative.Z_NEG) + this.ProcessGroundWaterFlowSide(chunkKey, _fromVoxelState, _mass1, WaterNeighborCacheNative.Z_POS) < 195)
        {
          _chunkData.SetVoxelInactive(current);
          ++this.stats.NumVoxelsPutToSleep;
        }
      }
      else
      {
        int _mass2 = _mass1;
        if (_mass2 < 195)
        {
          _chunkData.SetVoxelInactive(current);
          ++this.stats.NumVoxelsPutToSleep;
        }
        else
        {
          int num1 = _mass2;
          int _mass3 = _mass2 - this.ProcessFlowBelow(_chunkData, current, voxelCoords.x, voxelCoords.y, voxelCoords.z, _mass2);
          if (_mass3 > 0)
          {
            int _mass4 = _mass3 - this.ProcessOverfull(_chunkData, current, voxelCoords.x, voxelCoords.y, voxelCoords.z, _mass3);
            int num2 = this.ProcessFlowSide(chunkKey, _chunkData, current, _mass4, WaterNeighborCacheNative.X_NEG) + this.ProcessFlowSide(chunkKey, _chunkData, current, _mass4, WaterNeighborCacheNative.X_POS) + this.ProcessFlowSide(chunkKey, _chunkData, current, _mass4, WaterNeighborCacheNative.Z_NEG) + this.ProcessFlowSide(chunkKey, _chunkData, current, _mass4, WaterNeighborCacheNative.Z_POS);
            int num3 = _mass4 - num2;
            if (num3 > 0 && num1 - num3 < 195)
            {
              _chunkData.SetVoxelInactive(current);
              ++this.stats.NumVoxelsPutToSleep;
            }
          }
        }
      }
    }
    activeVoxelIndices.Dispose();
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public int ProcessFlowBelow(
    WaterDataHandle _chunkData,
    int _voxelIndex,
    int _x,
    int _y,
    int _z,
    int _mass)
  {
    int _y1 = _y - 1;
    if (_y1 < 0 || _chunkData.voxelState.Get(_x, _y, _z).IsSolidYNeg() || _chunkData.voxelState.Get(_x, _y1, _z).IsSolidYPos())
      return 0;
    if (_chunkData.IsInGroundWater(_x, _y1, _z))
    {
      _chunkData.ApplyFlow(_voxelIndex, -_mass);
      ++this.stats.NumFlowEvents;
      return _mass;
    }
    int _mass1;
    if (!this.TryGetMass(_chunkData, _x, _y1, _z, out _mass1))
      return 0;
    int num = WaterConstants.GetStableMassBelow(_mass, _mass1) - _mass1;
    if (num <= 0)
      return 0;
    int _flow = math.clamp((int) ((double) num * 0.5), 1, _mass);
    _chunkData.ApplyFlow(_voxelIndex, -_flow);
    _chunkData.ApplyFlow(_x, _y1, _z, _flow);
    ++this.stats.NumFlowEvents;
    return _flow;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public int ProcessOverfull(
    WaterDataHandle _chunkData,
    int _voxelIndex,
    int _x,
    int _y,
    int _z,
    int _mass)
  {
    if (_mass < 19500)
      return 0;
    int _y1 = _y + 1;
    int _mass1;
    if (_y1 > (int) byte.MaxValue || _chunkData.voxelState.Get(_x, _y, _z).IsSolidYPos() || _chunkData.voxelState.Get(_x, _y1, _z).IsSolidYNeg() || !this.TryGetMass(_chunkData, _x, _y1, _z, out _mass1))
      return 0;
    int x = math.min(_mass - 19500, 58500 - _mass1);
    if (x <= 195)
      return 0;
    int _flow = math.clamp(x, 1, _mass);
    _chunkData.ApplyFlow(_voxelIndex, -_flow);
    _chunkData.ApplyFlow(_x, _y1, _z, _flow);
    ++this.stats.NumFlowEvents;
    return _flow;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public int ProcessFlowSide(
    ChunkKey _chunkKey,
    WaterDataHandle _chunkData,
    int _voxelIndex,
    int _mass,
    int2 _xzOffset)
  {
    WaterVoxelState waterVoxelState1 = _chunkData.voxelState.Get(_voxelIndex);
    ChunkKey _chunkKey1;
    WaterDataHandle _dataHandle;
    int _x;
    int _y1;
    int _z;
    if (waterVoxelState1.IsSolidXZ(_xzOffset) || !this.neighborCache.TryGetNeighbor(_xzOffset, out _chunkKey1, out _dataHandle, out _x, out _y1, out _z))
      return 0;
    WaterVoxelState waterVoxelState2 = _dataHandle.voxelState.Get(_x, _y1, _z);
    int _mass1;
    if (waterVoxelState2.IsSolidXZ(-_xzOffset) || !this.TryGetMass(_dataHandle, _x, _y1, _z, out _mass1))
      return 0;
    int _y2 = _y1 - 1;
    bool flag1 = true;
    bool flag2 = true;
    if (_y2 >= 0)
    {
      flag1 = _chunkData.voxelState.Get(this.neighborCache.voxelX, _y2, this.neighborCache.voxelZ).IsSolidYPos() || waterVoxelState1.IsSolidYNeg();
      flag2 = _dataHandle.voxelState.Get(_x, _y2, _z).IsSolidYPos() || waterVoxelState2.IsSolidYNeg();
    }
    int num1 = 195;
    if (flag1 == flag2)
    {
      if (_mass <= 4875)
        return 0;
    }
    else
      num1 = 0;
    int num2 = math.clamp((int) ((double) (_mass - _mass1) * 0.5), 0, (int) ((double) _mass * 0.25));
    if (num2 <= num1)
      return 0;
    int _flow = math.clamp((int) ((double) num2 * 0.5), 1, _mass);
    _chunkData.ApplyFlow(_voxelIndex, -_flow);
    if (_chunkKey.Equals(_chunkKey1))
      _dataHandle.ApplyFlow(_x, _y1, _z, _flow);
    else
      _dataHandle.EnqueueFlow(WaterDataHandle.GetVoxelIndex(_x, _y1, _z), _flow);
    ++this.stats.NumFlowEvents;
    return _flow;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public int ProcessGroundWaterFlowSide(
    ChunkKey _chunkKey,
    WaterVoxelState _fromVoxelState,
    int _mass,
    int2 _xzOffset)
  {
    ChunkKey _chunkKey1;
    WaterDataHandle _dataHandle;
    int _x;
    int _y;
    int _z;
    int _mass1;
    if (_fromVoxelState.IsSolidXZ(_xzOffset) || !this.neighborCache.TryGetNeighbor(_xzOffset, out _chunkKey1, out _dataHandle, out _x, out _y, out _z) || _dataHandle.voxelState.Get(_x, _y, _z).IsSolidXZ(-_xzOffset) || !this.TryGetMass(_dataHandle, _x, _y, _z, out _mass1))
      return 0;
    int num1 = math.max(19500 - _mass1, 0);
    int num2 = math.min(_mass, (int) ((double) num1 * 0.25));
    if (num2 <= 195)
      return 0;
    int _flow = math.clamp((int) ((double) num2 * 0.5), 1, _mass);
    if (_chunkKey.Equals(_chunkKey1))
      _dataHandle.ApplyFlow(_x, _y, _z, _flow);
    else
      _dataHandle.EnqueueFlow(WaterDataHandle.GetVoxelIndex(_x, _y, _z), _flow);
    ++this.stats.NumFlowEvents;
    return _flow;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public bool TryGetMass(WaterDataHandle _chunkData, int _x, int _y, int _z, out int _mass)
  {
    if (_chunkData.IsInGroundWater(_x, _y, _z))
    {
      _mass = 19500;
      return false;
    }
    int num = _chunkData.voxelData.Get(_x, _y, _z);
    if (num > 195)
    {
      _mass = num;
      return true;
    }
    _mass = 0;
    return !_chunkData.voxelState.Get(_x, _y, _z).IsSolid();
  }
}
