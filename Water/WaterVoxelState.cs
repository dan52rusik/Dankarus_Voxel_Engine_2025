// Decompiled with JetBrains decompiler
// Type: WaterVoxelState
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using Unity.Mathematics;

#nullable disable
public struct WaterVoxelState : IEquatable<WaterVoxelState>
{
  [PublicizedFrom(EAccessModifier.Private)]
  public byte stateBits;

  public WaterVoxelState(byte stateBits) => this.stateBits = stateBits;

  public WaterVoxelState(WaterVoxelState other) => this.stateBits = other.stateBits;

  public bool IsDefault() => this.stateBits == (byte) 0;

  public bool IsSolidYPos() => ((uint) this.stateBits & 1U) > 0U;

  public bool IsSolidYNeg() => ((uint) this.stateBits & 2U) > 0U;

  public bool IsSolidXPos() => ((uint) this.stateBits & 32U /*0x20*/) > 0U;

  public bool IsSolidXNeg() => ((uint) this.stateBits & 8U) > 0U;

  public bool IsSolidZPos() => ((uint) this.stateBits & 4U) > 0U;

  public bool IsSolidZNeg() => ((uint) this.stateBits & 16U /*0x10*/) > 0U;

  public bool IsSolidXZ(int2 side)
  {
    if (side.x > 0)
      return this.IsSolidXPos();
    if (side.x < 0)
      return this.IsSolidXNeg();
    if (side.y > 0)
      return this.IsSolidZPos();
    return side.y < 0 ? this.IsSolidZNeg() : this.IsSolid();
  }

  public bool IsSolid() => this.stateBits != (byte) 0 && ((int) ~this.stateBits & 63 /*0x3F*/) == 0;

  public void SetSolid(BlockFaceFlag flags) => this.stateBits = (byte) flags;

  public void SetSolidMask(BlockFaceFlag mask, bool value)
  {
    if (value)
      this.stateBits |= (byte) mask;
    else
      this.stateBits &= (byte) ~mask;
  }

  public bool Equals(WaterVoxelState other) => (int) this.stateBits == (int) other.stateBits;
}
