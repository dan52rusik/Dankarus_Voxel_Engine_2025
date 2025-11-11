// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.PathShared
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using UnityEngine;

#nullable disable
namespace WorldGenerationEngineFinal;

public class PathShared
{
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly WorldBuilder worldBuilder;
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly Color32 CountryColor = new Color32((byte) 0, byte.MaxValue, (byte) 0, byte.MaxValue);
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly Color32 HighwayColor = new Color32(byte.MaxValue, (byte) 0, (byte) 0, byte.MaxValue);
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly Color32 WaterColor = new Color32((byte) 0, (byte) 0, byte.MaxValue, byte.MaxValue);
  public readonly Color32[] IdToColor;

  public PathShared(WorldBuilder _worldBuilder)
  {
    this.worldBuilder = _worldBuilder;
    Color32[] color32Array = new Color32[5];
    color32Array[1] = this.CountryColor;
    color32Array[2] = this.HighwayColor;
    color32Array[3] = this.CountryColor;
    color32Array[4] = this.WaterColor;
    this.IdToColor = color32Array;
  }

  public void ConvertIdsToColors(byte[] ids, Color32[] dest)
  {
    for (int index = 0; index < ids.Length; ++index)
    {
      int id = (int) ids[index];
      dest[index] = this.IdToColor[id & 15];
    }
  }
}
