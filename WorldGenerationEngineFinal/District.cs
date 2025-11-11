// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.District
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System.Collections.Generic;
using UnityEngine;

#nullable disable
namespace WorldGenerationEngineFinal;

public class District
{
  public string name;
  public string prefabName;
  public District.Type type;
  public FastTags<TagGroup.Poi> tag;
  public FastTags<TagGroup.Poi> townships;
  public float weight = 0.5f;
  public Color preview_color;
  public int counter;
  public bool spawnCustomSizePrefabs;
  public List<string> avoidedNeighborDistricts = new List<string>();

  public District()
  {
  }

  public District(District _other)
  {
    this.name = _other.name;
    this.prefabName = _other.prefabName;
    this.tag = _other.tag;
    this.townships = _other.townships;
    this.weight = _other.weight;
    this.preview_color = _other.preview_color;
    this.counter = _other.counter;
    this.avoidedNeighborDistricts = _other.avoidedNeighborDistricts;
    this.Init();
  }

  public void Init()
  {
    this.type = District.Type.None;
    if (this.name.EndsWith("commercial"))
      this.type = District.Type.Commercial;
    else if (this.name.EndsWith("downtown"))
      this.type = District.Type.Downtown;
    else if (this.name.EndsWith("gateway"))
    {
      this.type = District.Type.Gateway;
    }
    else
    {
      if (!this.name.EndsWith("rural"))
        return;
      this.type = District.Type.Rural;
    }
  }

  public enum Type
  {
    None,
    Commercial,
    Downtown,
    Gateway,
    Rural,
  }
}
