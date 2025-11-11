// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.TownshipShared
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

#nullable disable
namespace WorldGenerationEngineFinal;

public class TownshipShared
{
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly WorldBuilder worldBuilder;
  public int NextId;
  public readonly Vector2i[] dir4way = new Vector2i[4]
  {
    new Vector2i(0, 1),
    new Vector2i(1, 0),
    new Vector2i(0, -1),
    new Vector2i(-1, 0)
  };

  public TownshipShared(WorldBuilder _worldBuilder) => this.worldBuilder = _worldBuilder;
}
