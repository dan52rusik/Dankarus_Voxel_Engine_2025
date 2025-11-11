// Decompiled with JetBrains decompiler
// Type: WaterUtils
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

#nullable disable
public static class WaterUtils
{
  public static int GetVoxelKey2D(int _x, int _z) => _x * 8976890 + _z * 981131;

  public static int GetVoxelKey(int _x, int _y, int _z = 0) => _x * 8976890 + _y * 981131 + _z;

  public static bool IsChunkSafeToUpdate(Chunk chunk)
  {
    return chunk != null && !chunk.NeedsDecoration && !chunk.NeedsCopying && !chunk.IsLocked;
  }

  public static bool TryOpenChunkForUpdate(ChunkCluster _chunks, long _key, out Chunk _chunk)
  {
    using (ScopedChunkWriteAccess chunkWriteAccess = ScopedChunkAccess.GetChunkWriteAccess(_chunks, _key))
    {
      Chunk chunk = chunkWriteAccess.Chunk;
      if (!WaterUtils.IsChunkSafeToUpdate(chunk))
      {
        _chunk = (Chunk) null;
        return false;
      }
      _chunk = chunk;
      _chunk.InProgressWaterSim = true;
      return true;
    }
  }

  public static bool CanWaterFlowThrough(BlockValue _bv)
  {
    Block block = _bv.Block;
    return block != null && block.WaterFlowMask != BlockFaceFlag.All;
  }

  public static bool CanWaterFlowThrough(int _blockId)
  {
    Block block = Block.list[_blockId];
    return block != null && block.WaterFlowMask != BlockFaceFlag.All;
  }

  public static int GetWaterLevel(WaterValue waterValue) => waterValue.GetMass() > 195 ? 1 : 0;

  public static bool IsVoxelOutsideChunk(int _neighborX, int _neighborZ)
  {
    return _neighborX < 0 || _neighborX > 15 || _neighborZ < 0 || _neighborZ > 15;
  }
}
