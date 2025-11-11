// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.WorldGenerationFromXml
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using System.Collections;
using System.Collections.Generic;
using System.Xml.Linq;
using UnityEngine;

#nullable disable
namespace WorldGenerationEngineFinal;

public static class WorldGenerationFromXml
{
  public static void Cleanup()
  {
    WorldBuilderStatic.Properties.Clear();
    WorldBuilderStatic.WorldSizeMapper.Clear();
    WorldBuilderStatic.idToTownshipData.Clear();
    PrefabManagerStatic.prefabWeightData.Clear();
    PrefabManagerStatic.TileMinMaxCounts.Clear();
    PrefabManagerStatic.TileMaxDensityScore.Clear();
    DistrictPlannerStatic.Districts.Clear();
  }

  public static void Reload(XmlFile _xmlFile)
  {
    Debug.LogError((object) "Reloading world generation data!");
    WorldGenerationFromXml.Cleanup();
    ThreadManager.RunCoroutineSync(WorldGenerationFromXml.Load(_xmlFile));
  }

  public static IEnumerator Load(XmlFile file)
  {
    WorldGenerationFromXml.Cleanup();
    int _id = 0;
    XElement root = file.XmlDoc.Root;
    if (!root.HasElements)
      throw new Exception("No element <rwgmixer> found!");
    using (IEnumerator<XElement> enumerator = root.Elements().GetEnumerator())
    {
      while (enumerator.MoveNext())
      {
        XElement current = enumerator.Current;
        if (current.Name == (XName) "world")
        {
          XElement _element = current;
          if (_element.HasAttribute((XName) "name"))
          {
            string attribute = _element.GetAttribute((XName) "name");
            DynamicProperties dynamicProperties = new DynamicProperties();
            Vector2i one = Vector2i.one;
            foreach (XElement element in _element.Elements((XName) "property"))
            {
              dynamicProperties.Add(element);
              if (element.HasAttribute((XName) "name") && element.GetAttribute((XName) "name") == "world_size" && element.HasAttribute((XName) "value"))
                WorldBuilderStatic.WorldSizeMapper[attribute] = StringParsers.ParseVector2i(element.GetAttribute((XName) "value"));
            }
            WorldBuilderStatic.Properties[attribute] = dynamicProperties;
          }
        }
        else if (current.Name == (XName) "streettile")
        {
          XElement _element = current;
          string attribute1 = _element.GetAttribute((XName) "name");
          if (attribute1.Length != 0)
          {
            int _x = 0;
            int _y = int.MaxValue;
            int num = -1;
            foreach (XElement element in _element.Elements((XName) "property"))
            {
              string attribute2 = element.GetAttribute((XName) "name");
              string attribute3 = element.GetAttribute((XName) "value");
              if (attribute2.EqualsCaseInsensitive("maxtiles"))
                _y = StringParsers.ParseSInt32(attribute3);
              else if (attribute2.EqualsCaseInsensitive("mintiles"))
                _x = StringParsers.ParseSInt32(attribute3);
              else if (attribute2.EqualsCaseInsensitive("maxdensity"))
                num = StringParsers.ParseSInt32(attribute3);
            }
            if (_x > 0 || _y != int.MaxValue)
              PrefabManagerStatic.TileMinMaxCounts[attribute1] = new Vector2i(_x, _y);
            if (num >= 0)
              PrefabManagerStatic.TileMaxDensityScore[attribute1] = (float) num;
          }
        }
        else if (current.Name == (XName) "district")
        {
          XElement _element = current;
          District district = new District()
          {
            name = _element.GetAttribute((XName) "name").ToLower()
          };
          district.prefabName = district.name;
          district.tag = FastTags<TagGroup.Poi>.Parse(district.name);
          foreach (XElement element in _element.Elements((XName) "property"))
          {
            string attribute = element.GetAttribute((XName) "name");
            string lower = element.GetAttribute((XName) "value").ToLower();
            if (attribute.Length > 0 && lower.Length > 0)
            {
              if (attribute.EqualsCaseInsensitive("prefab_name"))
                district.prefabName = lower;
              else if (attribute.EqualsCaseInsensitive("tag"))
                district.tag = FastTags<TagGroup.Poi>.Parse(lower);
              else if (attribute.EqualsCaseInsensitive("spawn_weight"))
                district.weight = StringParsers.ParseFloat(lower);
              else if (attribute.EqualsCaseInsensitive("required_township"))
                district.townships = FastTags<TagGroup.Poi>.Parse(lower);
              else if (attribute.EqualsCaseInsensitive("preview_color"))
                district.preview_color = StringParsers.ParseColor(lower);
              else if (attribute.EqualsCaseInsensitive("spawn_custom_size_prefabs"))
                district.spawnCustomSizePrefabs = StringParsers.ParseBool(lower);
              else if (attribute.EqualsCaseInsensitive("avoided_neighbor_districts"))
                district.avoidedNeighborDistricts = new List<string>((IEnumerable<string>) lower.Split(',', StringSplitOptions.None));
            }
          }
          district.prefabName += "_";
          district.Init();
          DistrictPlannerStatic.Districts[district.name] = district;
        }
        else if (current.Name == (XName) "township")
        {
          TownshipData townshipData = new TownshipData(current.GetAttribute((XName) "name"), _id);
          ++_id;
          foreach (XElement element in current.Elements((XName) "property"))
          {
            string attribute4 = element.GetAttribute((XName) "name");
            string attribute5 = element.GetAttribute((XName) "value");
            if (attribute4.Length > 0 && attribute5.Length > 0)
            {
              if (attribute4.EqualsCaseInsensitive("spawnable_terrain"))
                townshipData.SpawnableTerrain.AddRange((IEnumerable<string>) attribute5.Replace(" ", "").Split(',', StringSplitOptions.None));
              else if (attribute4.EqualsCaseInsensitive("outskirt_district"))
              {
                string[] strArray = attribute5.Split(",", StringSplitOptions.None);
                townshipData.OutskirtDistrict = strArray[0];
                townshipData.OutskirtDistrictPercent = strArray.Length >= 2 ? float.Parse(strArray[1]) : 1f;
              }
              else if (attribute4.EqualsCaseInsensitive("spawn_custom_size_prefabs"))
                townshipData.SpawnCustomSizes = StringParsers.ParseBool(attribute5);
              else if (attribute4.EqualsCaseInsensitive("spawn_trader"))
                townshipData.SpawnTrader = StringParsers.ParseBool(attribute5);
              else if (attribute4.EqualsCaseInsensitive("spawn_gateway"))
                townshipData.SpawnGateway = StringParsers.ParseBool(attribute5);
              else if (attribute4.EqualsCaseInsensitive("biomes"))
                townshipData.Biomes = FastTags<TagGroup.Poi>.Parse(attribute5.ToLower());
            }
          }
        }
        else if (current.Name == (XName) "prefab_spawn_adjust")
        {
          FastTags<TagGroup.Poi> none1 = FastTags<TagGroup.Poi>.none;
          FastTags<TagGroup.Poi> none2 = FastTags<TagGroup.Poi>.none;
          float _weight = 1f;
          float _bias = 0.0f;
          int maxCount = int.MaxValue;
          int minCount = 1;
          string attribute = current.GetAttribute((XName) "partial_name");
          if (current.HasAttribute((XName) "tags"))
            none1 = FastTags<TagGroup.Poi>.Parse(current.GetAttribute((XName) "tags"));
          string _result;
          if (current.TryGetAttribute((XName) "biomeTags", out _result))
            none2 = FastTags<TagGroup.Poi>.Parse(_result);
          if (current.HasAttribute((XName) "weight"))
            _weight = StringParsers.ParseFloat(current.GetAttribute((XName) "weight"));
          if (current.HasAttribute((XName) "bias"))
            _bias = StringParsers.ParseFloat(current.GetAttribute((XName) "bias"));
          if (current.HasAttribute((XName) "min_count"))
            minCount = StringParsers.ParseSInt32(current.GetAttribute((XName) "min_count"));
          if (current.HasAttribute((XName) "max_count"))
            maxCount = StringParsers.ParseSInt32(current.GetAttribute((XName) "max_count"));
          if (!string.IsNullOrEmpty(attribute) || !none1.IsEmpty)
            PrefabManagerStatic.prefabWeightData.Add(new PrefabManager.POIWeightData(attribute, none1, none2, _weight, _bias, minCount, maxCount));
        }
      }
      yield break;
    }
  }
}
