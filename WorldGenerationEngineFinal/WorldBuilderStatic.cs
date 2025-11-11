// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.WorldBuilderStatic
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System.Collections.Generic;

#nullable disable
namespace WorldGenerationEngineFinal;

public static class WorldBuilderStatic
{
  public static readonly Dictionary<string, DynamicProperties> Properties = new Dictionary<string, DynamicProperties>();
  public static readonly Dictionary<string, Vector2i> WorldSizeMapper = new Dictionary<string, Vector2i>();
  public static readonly Dictionary<int, TownshipData> idToTownshipData = new Dictionary<int, TownshipData>();

  [PublicizedFrom(EAccessModifier.Private)]
  static WorldBuilderStatic()
  {
  }
}
