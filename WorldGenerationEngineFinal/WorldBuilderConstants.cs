// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.WorldBuilderConstants
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System.Collections.Generic;
using UnityEngine;

#nullable disable
namespace WorldGenerationEngineFinal;

public static class WorldBuilderConstants
{
  public static readonly Color32 forestCol = new Color32((byte) 0, (byte) 64 /*0x40*/, (byte) 0, byte.MaxValue);
  public static readonly Color32 burntForestCol = new Color32((byte) 186, (byte) 0, byte.MaxValue, byte.MaxValue);
  public static readonly Color32 desertCol = new Color32(byte.MaxValue, (byte) 228, (byte) 119, byte.MaxValue);
  public static readonly Color32 snowCol = new Color32(byte.MaxValue, byte.MaxValue, byte.MaxValue, byte.MaxValue);
  public static readonly Color32 wastelandCol = new Color32(byte.MaxValue, (byte) 168, (byte) 0, byte.MaxValue);
  public static readonly List<Color32> biomeColorList = new List<Color32>()
  {
    WorldBuilderConstants.forestCol,
    WorldBuilderConstants.burntForestCol,
    WorldBuilderConstants.desertCol,
    WorldBuilderConstants.snowCol,
    WorldBuilderConstants.wastelandCol
  };
  public const int ForestBiomeWeightDefault = 13;
  public const int BurntForestBiomeWeightDefault = 18;
  public const int DesertBiomeWeightDefault = 22;
  public const int SnowBiomeWeightDefault = 23;
  public const int WastelandBiomeWeightDefault = 24;
  public static readonly int[] BiomeWeightDefaults = new int[5]
  {
    13,
    18,
    22,
    23,
    24
  };

  [PublicizedFrom(EAccessModifier.Private)]
  static WorldBuilderConstants()
  {
  }
}
