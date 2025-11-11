// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.TownshipData
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System.Collections.Generic;

#nullable disable
namespace WorldGenerationEngineFinal;

public class TownshipData
{
  public string Name;
  public int Id;
  public List<string> SpawnableTerrain = new List<string>();
  public bool SpawnCustomSizes;
  public bool SpawnTrader = true;
  public bool SpawnGateway = true;
  public string OutskirtDistrict;
  public float OutskirtDistrictPercent;
  public FastTags<TagGroup.Poi> Biomes;
  public TownshipData.eCategory Category;

  public TownshipData(string _name, int _id)
  {
    this.Name = _name;
    this.Id = _id;
    if (_name.EndsWith("roadside"))
      this.Category = TownshipData.eCategory.Roadside;
    else if (_name.EndsWith("rural"))
      this.Category = TownshipData.eCategory.Rural;
    else if (_name.EndsWith("wilderness"))
      this.Category = TownshipData.eCategory.Wilderness;
    WorldBuilderStatic.idToTownshipData[this.Id] = this;
  }

  public enum eCategory
  {
    Normal,
    Roadside,
    Rural,
    Wilderness,
  }
}
