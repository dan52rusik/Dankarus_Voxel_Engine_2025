// Decompiled with JetBrains decompiler
// Type: WaterNeighborCacheNative
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using Unity.Collections.LowLevel.Unsafe;
using Unity.Mathematics;

#nullable disable
public struct WaterNeighborCacheNative
{
  public static readonly int2 X_NEG = new int2(-1, 0);
  public static readonly int2 X_POS = new int2(1, 0);
  public static readonly int2 Z_NEG = new int2(0, -1);
  public static readonly int2 Z_POS = new int2(0, 1);
  [PublicizedFrom(EAccessModifier.Private)]
  public UnsafeParallelHashMap<ChunkKey, WaterDataHandle> waterDataHandles;
  public ChunkKey chunkKey;
  public int voxelX;
  public int voxelY;
  public int voxelZ;
  public WaterDataHandle center;

  public static WaterNeighborCacheNative InitializeCache(
    UnsafeParallelHashMap<ChunkKey, WaterDataHandle> _handles)
  {
    return new WaterNeighborCacheNative()
    {
      waterDataHandles = _handles
    };
  }

  public void SetChunk(ChunkKey _chunk)
  {
    this.chunkKey = _chunk;
    this.center = this.waterDataHandles[_chunk];
  }

  public void SetVoxel(int _x, int _y, int _z)
  {
    this.voxelX = _x;
    this.voxelY = _y;
    this.voxelZ = _z;
  }

  public bool TryGetNeighbor(
    int2 _xzOffset,
    out ChunkKey _chunkKey,
    out WaterDataHandle _dataHandle,
    out int _x,
    out int _y,
    out int _z)
  {
    _x = this.voxelX + _xzOffset.x;
    _y = this.voxelY;
    _z = this.voxelZ + _xzOffset.y;
    if (!WaterUtils.IsVoxelOutsideChunk(_x, _z))
    {
      _chunkKey = this.chunkKey;
      _dataHandle = this.center;
      return true;
    }
    int num1 = _x & 15;
    int num2 = _z & 15;
    int _x1 = this.chunkKey.x + (_x - num1) / 16 /*0x10*/;
    int _z1 = this.chunkKey.z + (_z - num2) / 16 /*0x10*/;
    _chunkKey = new ChunkKey(_x1, _z1);
    if (this.waterDataHandles.TryGetValue(_chunkKey, out _dataHandle))
    {
      _x = num1;
      _z = num2;
      return true;
    }
    _chunkKey = new ChunkKey();
    _dataHandle = new WaterDataHandle();
    return false;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  static WaterNeighborCacheNative()
  {
  }
}
