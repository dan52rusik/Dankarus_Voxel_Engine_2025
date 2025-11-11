// Decompiled with JetBrains decompiler
// Type: WaterDataHandle
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using Unity.Collections;
using Unity.Collections.LowLevel.Unsafe;
using Unity.Mathematics;
using UnityEngine;

#nullable disable
public struct WaterDataHandle : IDisposable
{
  public UnsafeChunkData<int> voxelData;
  public UnsafeChunkData<WaterVoxelState> voxelState;
  public UnsafeChunkXZMap<GroundWaterBounds> groundWaterHeights;
  public UnsafeBitArray activeVoxels;
  public UnsafeParallelHashMap<int, int> flowVoxels;
  public UnsafeFixedBuffer<WaterFlow> flowsFromOtherChunks;
  public UnsafeFixedBuffer<int> activationsFromOtherChunks;
  public UnsafeParallelHashSet<int> voxelsToWakeup;

  public UnsafeBitArraySetIndicesEnumerator ActiveVoxelIndices
  {
    get => new UnsafeBitArraySetIndicesEnumerator(this.activeVoxels);
  }

  public UnsafeParallelHashMap<int, int>.Enumerator FlowVoxels => this.flowVoxels.GetEnumerator();

  public bool HasActiveWater => this.activeVoxels.TestAny(0, this.activeVoxels.Length);

  public bool HasFlows => !this.flowVoxels.IsEmpty;

  public static WaterDataHandle AllocateNew(Allocator allocator)
  {
    WaterDataHandle waterDataHandle = new WaterDataHandle();
    waterDataHandle.Allocate(allocator);
    return waterDataHandle;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void Allocate(Allocator allocator)
  {
    this.voxelData = new UnsafeChunkData<int>((AllocatorManager.AllocatorHandle) allocator);
    this.voxelState = new UnsafeChunkData<WaterVoxelState>((AllocatorManager.AllocatorHandle) allocator);
    this.groundWaterHeights = new UnsafeChunkXZMap<GroundWaterBounds>((AllocatorManager.AllocatorHandle) allocator);
    this.activeVoxels = new UnsafeBitArray(65536 /*0x010000*/, (AllocatorManager.AllocatorHandle) allocator);
    this.flowVoxels = new UnsafeParallelHashMap<int, int>(1000, (AllocatorManager.AllocatorHandle) allocator);
    this.flowsFromOtherChunks = new UnsafeFixedBuffer<WaterFlow>(16384 /*0x4000*/, (AllocatorManager.AllocatorHandle) allocator);
    this.activationsFromOtherChunks = new UnsafeFixedBuffer<int>(16384 /*0x4000*/, (AllocatorManager.AllocatorHandle) allocator);
    this.voxelsToWakeup = new UnsafeParallelHashSet<int>(256 /*0x0100*/, (AllocatorManager.AllocatorHandle) allocator);
  }

  public bool IsInGroundWater(int _x, int _y, int _z)
  {
    GroundWaterBounds groundWaterBounds = this.groundWaterHeights.Get(_x, _z);
    return groundWaterBounds.IsGroundWater && _y >= (int) groundWaterBounds.bottom && _y <= (int) groundWaterBounds.waterHeight;
  }

  public void SetVoxelActive(int _x, int _y, int _z)
  {
    this.activeVoxels.Set(WaterDataHandle.GetVoxelIndex(_x, _y, _z), true);
  }

  public void SetVoxelActive(int _index) => this.activeVoxels.Set(_index, true);

  public void EnqueueVoxelActive(int _x, int _y, int _z)
  {
    this.EnqueueVoxelActive(WaterDataHandle.GetVoxelIndex(_x, _y, _z));
  }

  public void EnqueueVoxelActive(int _index)
  {
    this.activationsFromOtherChunks.AddThreadSafe(_index);
  }

  public void EnqueueVoxelWakeup(int _x, int _y, int _z)
  {
    this.EnqueueVoxelWakeup(WaterDataHandle.GetVoxelIndex(_x, _y, _z));
  }

  public void EnqueueVoxelWakeup(int _index) => this.voxelsToWakeup.Add(_index);

  public void ApplyEnqueuedActivations()
  {
    NativeArray<int> nativeArray = this.activationsFromOtherChunks.AsNativeArray();
    for (int index = 0; index < nativeArray.Length; ++index)
      this.SetVoxelActive(nativeArray[index]);
    this.activationsFromOtherChunks.Clear();
  }

  public void SetVoxelInactive(int _index) => this.activeVoxels.Set(_index, false);

  public void SetVoxelMass(int _x, int _y, int _z, int _mass)
  {
    this.SetVoxelMass(WaterDataHandle.GetVoxelIndex(_x, _y, _z), _mass);
  }

  public void SetVoxelMass(int _index, int _mass)
  {
    if (_mass > 195)
      this.activeVoxels.Set(_index, true);
    else
      this.activeVoxels.Set(_index, false);
    this.voxelData.Set(_index, _mass);
  }

  public void SetVoxelSolid(int _x, int _y, int _z, BlockFaceFlag _flags)
  {
    int voxelIndex = WaterDataHandle.GetVoxelIndex(_x, _y, _z);
    WaterVoxelState waterVoxelState1 = this.voxelState.Get(voxelIndex);
    WaterVoxelState waterVoxelState2 = new WaterVoxelState();
    waterVoxelState2.SetSolid(_flags);
    this.voxelState.Set(voxelIndex, waterVoxelState2);
    GroundWaterBounds groundWaterBounds = this.groundWaterHeights.Get(_x, _z);
    if (!groundWaterBounds.IsGroundWater)
      return;
    if (waterVoxelState1.IsSolidYNeg() && !waterVoxelState2.IsSolidYNeg() && _y == (int) groundWaterBounds.bottom)
    {
      groundWaterBounds.bottom = (byte) this.FindGroundWaterBottom(voxelIndex);
      this.groundWaterHeights.Set(_x, _z, groundWaterBounds);
    }
    else if (waterVoxelState1.IsSolidYPos() && !waterVoxelState2.IsSolidYPos() && _y + 1 == (int) groundWaterBounds.bottom)
    {
      groundWaterBounds.bottom = (byte) this.FindGroundWaterBottom(voxelIndex);
      this.groundWaterHeights.Set(_x, _z, groundWaterBounds);
    }
    else if (!waterVoxelState1.IsSolidYNeg() && waterVoxelState2.IsSolidYNeg() && _y > (int) groundWaterBounds.bottom && _y <= (int) groundWaterBounds.waterHeight)
    {
      groundWaterBounds.bottom = (byte) _y;
      this.groundWaterHeights.Set(_x, _z, groundWaterBounds);
    }
    else
    {
      if (waterVoxelState1.IsSolidYPos() || !waterVoxelState2.IsSolidYPos())
        return;
      int num = _y + 1;
      if (num <= (int) groundWaterBounds.bottom || num > (int) groundWaterBounds.waterHeight)
        return;
      groundWaterBounds.bottom = (byte) num;
      this.groundWaterHeights.Set(_x, _z, groundWaterBounds);
    }
  }

  public void ApplyFlow(int _x, int _y, int _z, int _flow)
  {
    this.ApplyFlow(WaterDataHandle.GetVoxelIndex(_x, _y, _z), _flow);
  }

  public void ApplyFlow(int _index, int _flow)
  {
    int num;
    if (this.flowVoxels.TryGetValue(_index, out num))
      _flow += num;
    this.flowVoxels[_index] = _flow;
  }

  public void EnqueueFlow(int _voxelIndex, int _flow)
  {
    this.flowsFromOtherChunks.AddThreadSafe(new WaterFlow()
    {
      voxelIndex = _voxelIndex,
      flow = _flow
    });
  }

  public void ApplyEnqueuedFlows()
  {
    NativeArray<WaterFlow> nativeArray = this.flowsFromOtherChunks.AsNativeArray();
    for (int index = 0; index < nativeArray.Length; ++index)
    {
      WaterFlow waterFlow = nativeArray[index];
      this.ApplyFlow(waterFlow.voxelIndex, waterFlow.flow);
    }
    this.flowsFromOtherChunks.Clear();
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public int FindGroundWaterBottom(int _fromIndex)
  {
    for (int index = _fromIndex; index >= 0; index -= 256 /*0x0100*/)
    {
      WaterVoxelState waterVoxelState = this.voxelState.Get(index);
      if (waterVoxelState.IsSolidYNeg())
        return WaterDataHandle.GetVoxelY(index);
      if (waterVoxelState.IsSolidYPos())
      {
        int _index = math.min(index + 256 /*0x0100*/, (int) byte.MaxValue);
        if (_index <= _fromIndex)
          return WaterDataHandle.GetVoxelY(_index);
      }
    }
    return 0;
  }

  public void InitializeFromChunk(Chunk _chunk, GroundWaterHeightMap _groundWaterHeightMap)
  {
    if (!this.voxelData.IsCreated || !this.activeVoxels.IsCreated)
    {
      Debug.LogError((object) "Could not initialize WaterDataHandle because it has not been allocated");
    }
    else
    {
      this.Clear();
      for (int _y = 0; _y < 256 /*0x0100*/; ++_y)
      {
        for (int _z = 0; _z < 16 /*0x10*/; ++_z)
        {
          for (int _x = 0; _x < 16 /*0x10*/; ++_x)
          {
            WaterVoxelState waterVoxelState = new WaterVoxelState();
            BlockValue blockNoDamage = _chunk.GetBlockNoDamage(_x, _y, _z);
            Block block = blockNoDamage.Block;
            byte rotation = blockNoDamage.rotation;
            waterVoxelState.SetSolid(BlockFaceFlags.RotateFlags(block.WaterFlowMask, rotation));
            int voxelIndex = WaterDataHandle.GetVoxelIndex(_x, _y, _z);
            int mass = _chunk.GetWater(_x, _y, _z).GetMass();
            if (mass > 195)
            {
              this.activeVoxels.Set(voxelIndex, true);
              this.voxelData.Set(voxelIndex, mass);
            }
            if (!waterVoxelState.IsDefault())
              this.voxelState.Set(voxelIndex, waterVoxelState);
          }
        }
      }
      this.voxelState.CheckSameValues();
      if (!_groundWaterHeightMap.TryInit())
        return;
      for (int _z = 0; _z < 16 /*0x10*/; ++_z)
      {
        for (int _x = 0; _x < 16 /*0x10*/; ++_x)
        {
          Vector3i worldPos = _chunk.ToWorldPos(_x, 0, _z);
          int _height;
          if (_groundWaterHeightMap.TryGetWaterHeightAt(worldPos.x, worldPos.z, out _height))
          {
            int groundWaterBottom = this.FindGroundWaterBottom(WaterDataHandle.GetVoxelIndex(_x, _height, _z));
            this.groundWaterHeights.Set(_x, _z, new GroundWaterBounds(groundWaterBottom, _height));
          }
        }
      }
    }
  }

  public void Clear()
  {
    if (this.voxelData.IsCreated)
      this.voxelData.Clear();
    if (this.voxelState.IsCreated)
      this.voxelState.Clear();
    if (this.groundWaterHeights.IsCreated)
      this.groundWaterHeights.Clear();
    if (this.activeVoxels.IsCreated)
      this.activeVoxels.Clear();
    if (this.flowVoxels.IsCreated)
      this.flowVoxels.Clear();
    if (this.flowsFromOtherChunks.IsCreated)
      this.flowsFromOtherChunks.Clear();
    if (this.activationsFromOtherChunks.IsCreated)
      this.activationsFromOtherChunks.Clear();
    if (!this.voxelsToWakeup.IsCreated)
      return;
    this.voxelsToWakeup.Clear();
  }

  public void Dispose()
  {
    if (this.voxelData.IsCreated)
      this.voxelData.Dispose();
    if (this.voxelState.IsCreated)
      this.voxelState.Dispose();
    if (this.groundWaterHeights.IsCreated)
      this.groundWaterHeights.Dispose();
    if (this.activeVoxels.IsCreated)
      this.activeVoxels.Dispose();
    if (this.flowVoxels.IsCreated)
      this.flowVoxels.Dispose();
    if (this.flowsFromOtherChunks.IsCreated)
      this.flowsFromOtherChunks.Dispose();
    if (this.activationsFromOtherChunks.IsCreated)
      this.activationsFromOtherChunks.Dispose();
    if (!this.voxelsToWakeup.IsCreated)
      return;
    this.voxelsToWakeup.Dispose();
  }

  public int CalculateOwnedBytes()
  {
    return 0 + this.voxelData.CalculateOwnedBytes() + this.voxelState.CalculateOwnedBytes() + this.groundWaterHeights.CalculateOwnedBytes() + ProfilerUtils.CalculateUnsafeBitArrayBytes(this.activeVoxels) + ProfilerUtils.CalculateUnsafeParallelHashMapBytes<int, int>(this.flowVoxels) + this.flowsFromOtherChunks.CalculateOwnedBytes() + this.activationsFromOtherChunks.CalculateOwnedBytes() + ProfilerUtils.CalculateUnsafeParallelHashSetBytes<int>(this.voxelsToWakeup);
  }

  public string GetMemoryStats()
  {
    return $"voxelData: {(double) this.voxelData.CalculateOwnedBytes() * 0.0009765625:F2} KB, voxelState: {(double) this.voxelState.CalculateOwnedBytes() * 0.0009765625:F2} KB, groundWaterHeights: {(double) this.groundWaterHeights.CalculateOwnedBytes() * 0.0009765625:F2} KB, activeVoxels: ({(double) ProfilerUtils.CalculateUnsafeBitArrayBytes(this.activeVoxels) * 0.0009765625:F2} KB), flowVoxels: ({this.flowVoxels.Count()},{this.flowVoxels.Capacity},{(double) ProfilerUtils.CalculateUnsafeParallelHashMapBytes<int, int>(this.flowVoxels) * 0.0009765625:F2} KB), flowsFromOtherChunks: {(double) this.flowsFromOtherChunks.CalculateOwnedBytes() * 0.0009765625:F2} KB, activationsFromOtherChunks: {(double) this.activationsFromOtherChunks.CalculateOwnedBytes() * 0.0009765625:F2} KB, voxelsToWakeup {(double) ProfilerUtils.CalculateUnsafeParallelHashSetBytes<int>(this.voxelsToWakeup) * 0.0009765625:F2} KB, Total: {(double) this.CalculateOwnedBytes() * 9.5367431640625E-07:F2} MB";
  }

  public static int GetVoxelIndex(int _x, int _y, int _z)
  {
    return _x + _y * 256 /*0x0100*/ + _z * 16 /*0x10*/;
  }

  public static int3 GetVoxelCoords(int index)
  {
    int3 voxelCoords = new int3();
    voxelCoords.y = index / 256 /*0x0100*/;
    int num = index % 256 /*0x0100*/;
    voxelCoords.z = num / 16 /*0x10*/;
    voxelCoords.x = num % 16 /*0x10*/;
    return voxelCoords;
  }

  public static int GetVoxelY(int _index) => _index / 256 /*0x0100*/;
}
