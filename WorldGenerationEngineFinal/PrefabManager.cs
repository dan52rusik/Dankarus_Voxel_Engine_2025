// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.PrefabManager
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Xml;
using UniLinq;
using UnityEngine;

#nullable disable
namespace WorldGenerationEngineFinal;

public class PrefabManager
{
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly WorldBuilder worldBuilder;
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly PrefabManagerData prefabManagerData = new PrefabManagerData();
  public readonly Dictionary<string, int> StreetTilesUsed = new Dictionary<string, int>();
  public readonly List<PrefabDataInstance> UsedPrefabsWorld = new List<PrefabDataInstance>();
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly Dictionary<string, int> WorldUsedPrefabNames = new Dictionary<string, int>();
  public int PrefabInstanceId;

  public PrefabManager(WorldBuilder _worldBuilder) => this.worldBuilder = _worldBuilder;

  public IEnumerator LoadPrefabs()
  {
    this.ClearDisplayed();
    yield return (object) this.prefabManagerData.LoadPrefabs();
  }

  public void ShufflePrefabData(int _seed) => this.prefabManagerData.ShufflePrefabData(_seed);

  public void Clear() => this.StreetTilesUsed.Clear();

  public void ClearDisplayed()
  {
    this.UsedPrefabsWorld.Clear();
    this.WorldUsedPrefabNames.Clear();
  }

  public void Cleanup()
  {
    this.prefabManagerData.Cleanup();
    this.ClearDisplayed();
  }

  public static bool isSizeValid(PrefabData prefab, Vector2i minSize, Vector2i maxSize)
  {
    if (!(maxSize == new Vector2i()) && (prefab.size.x > maxSize.x || prefab.size.z > maxSize.y) && (prefab.size.z > maxSize.x || prefab.size.x > maxSize.y))
      return false;
    if (minSize == new Vector2i() || prefab.size.x >= minSize.x && prefab.size.z >= minSize.y)
      return true;
    return prefab.size.z >= minSize.x && prefab.size.x >= minSize.y;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public bool isThemeValid(
    PrefabData prefab,
    Vector2i prefabPos,
    List<PrefabDataInstance> prefabInstances,
    int distance)
  {
    if (prefab.ThemeTags.IsEmpty)
      return true;
    prefabPos.x -= this.worldBuilder.WorldSize / 2;
    prefabPos.y -= this.worldBuilder.WorldSize / 2;
    int num = distance * distance;
    foreach (PrefabDataInstance prefabInstance in prefabInstances)
    {
      if (!prefabInstance.prefab.ThemeTags.IsEmpty && prefabInstance.prefab.ThemeTags.Test_AnySet(prefab.ThemeTags) && (double) Vector2i.DistanceSqr(prefabInstance.CenterXZ, prefabPos) < (double) num)
        return false;
    }
    return true;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public bool isNameValid(
    PrefabData prefab,
    Vector2i prefabPos,
    List<PrefabDataInstance> prefabInstances,
    int distance)
  {
    prefabPos.x -= this.worldBuilder.WorldSize / 2;
    prefabPos.y -= this.worldBuilder.WorldSize / 2;
    int num = distance * distance;
    foreach (PrefabDataInstance prefabInstance in prefabInstances)
    {
      if (!(prefabInstance.prefab.Name != prefab.Name) && (double) Vector2i.DistanceSqr(prefabInstance.CenterXZ, prefabPos) < (double) num)
        return false;
    }
    return true;
  }

  public PrefabData GetPrefabWithDistrict(
    District _district,
    FastTags<TagGroup.Poi> _markerTags,
    Vector2i minSize,
    Vector2i maxSize,
    Vector2i center,
    float densityPointsLeft,
    float _distanceScale)
  {
    bool flag1 = !_district.tag.IsEmpty;
    bool flag2 = !_markerTags.IsEmpty;
    PrefabData prefabWithDistrict = (PrefabData) null;
    float num = float.MinValue;
    int worldSizeDistDiv = this.worldBuilder.WorldSizeDistDiv;
    for (int index = 0; index < this.prefabManagerData.prefabDataList.Count; ++index)
    {
      PrefabData prefabData = this.prefabManagerData.prefabDataList[index];
      if ((double) prefabData.DensityScore <= (double) densityPointsLeft && !prefabData.Tags.Test_AnySet(this.prefabManagerData.PartsAndTilesTags) && (!flag1 || prefabData.Tags.Test_AllSet(_district.tag)) && (!flag2 || prefabData.Tags.Test_AnySet(_markerTags)) && PrefabManager.isSizeValid(prefabData, minSize, maxSize))
      {
        int themeRepeatDistance = prefabData.ThemeRepeatDistance;
        if (prefabData.ThemeTags.Test_AnySet(this.prefabManagerData.TraderTags))
          themeRepeatDistance /= worldSizeDistDiv;
        if (this.isThemeValid(prefabData, center, this.UsedPrefabsWorld, themeRepeatDistance) && ((double) _distanceScale <= 0.0 || this.isNameValid(prefabData, center, this.UsedPrefabsWorld, (int) ((double) prefabData.DuplicateRepeatDistance * (double) _distanceScale))))
        {
          float scoreForPrefab = this.getScoreForPrefab(prefabData, center);
          if ((double) scoreForPrefab > (double) num)
          {
            num = scoreForPrefab;
            prefabWithDistrict = prefabData;
          }
        }
      }
    }
    return prefabWithDistrict;
  }

  public PrefabData GetWildernessPrefab(
    FastTags<TagGroup.Poi> _withoutTags,
    FastTags<TagGroup.Poi> _markerTags,
    Vector2i minSize = default (Vector2i),
    Vector2i maxSize = default (Vector2i),
    Vector2i center = default (Vector2i),
    bool _isRetry = false)
  {
    PrefabData prefabData1 = (PrefabData) null;
    float num = float.MinValue;
    for (int index = 0; index < this.prefabManagerData.prefabDataList.Count; ++index)
    {
      PrefabData prefabData2 = this.prefabManagerData.prefabDataList[index];
      if (!prefabData2.Tags.Test_AnySet(this.prefabManagerData.PartsAndTilesTags) && (prefabData2.Tags.Test_AnySet(this.prefabManagerData.WildernessTags) || prefabData2.Tags.Test_AnySet(this.prefabManagerData.TraderTags)) && (_markerTags.IsEmpty || prefabData2.Tags.Test_AnySet(_markerTags) || prefabData2.ThemeTags.Test_AnySet(_markerTags)) && PrefabManager.isSizeValid(prefabData2, minSize, maxSize) && this.isThemeValid(prefabData2, center, this.UsedPrefabsWorld, prefabData2.ThemeRepeatDistance) && (_isRetry || this.isNameValid(prefabData2, center, this.UsedPrefabsWorld, prefabData2.DuplicateRepeatDistance)))
      {
        float scoreForPrefab = this.getScoreForPrefab(prefabData2, center);
        if ((double) scoreForPrefab > (double) num)
        {
          num = scoreForPrefab;
          prefabData1 = prefabData2;
        }
      }
    }
    return prefabData1 == null && !_isRetry ? this.GetWildernessPrefab(_withoutTags, _markerTags, minSize, maxSize, center, true) : prefabData1;
  }

  public static void Shuffle<T>(int seed, ref List<T> list)
  {
    GameRandom gameRandom = GameRandomManager.Instance.CreateGameRandom(seed);
    int count = list.Count;
    while (count > 1)
    {
      --count;
      int index = gameRandom.RandomRange(0, count);
      T obj = list[index];
      list[index] = list[count];
      list[count] = obj;
    }
    GameRandomManager.Instance.FreeGameRandom(gameRandom);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static int getRandomVal(int min, int maxExclusive, int seed)
  {
    GameRandom gameRandom = GameRandomManager.Instance.CreateGameRandom(seed);
    int randomVal = gameRandom.RandomRange(min, maxExclusive);
    GameRandomManager.Instance.FreeGameRandom(gameRandom);
    return randomVal;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static float getRandomVal(float min, float max, int seed)
  {
    GameRandom gameRandom = GameRandomManager.Instance.CreateGameRandom(seed);
    double randomVal = (double) gameRandom.RandomRange(min, max);
    GameRandomManager.Instance.FreeGameRandom(gameRandom);
    return (float) randomVal;
  }

  public PrefabData GetPrefabByName(string _lowerCaseName)
  {
    PrefabData prefabData;
    return !this.prefabManagerData.AllPrefabDatas.TryGetValue(_lowerCaseName.ToLower(), out prefabData) ? (PrefabData) null : prefabData;
  }

  public PrefabData GetStreetTile(string _lowerCaseName, Vector2i centerPoint, bool useExactString = false)
  {
    GameRandom rnd = GameRandomManager.Instance.CreateGameRandom(this.worldBuilder.Seed + (centerPoint.x + centerPoint.x * centerPoint.y * centerPoint.y));
    Vector2i vector2i1;
    string key = this.prefabManagerData.AllPrefabDatas.Keys.Where<string>((Func<string, bool>) ([PublicizedFrom(EAccessModifier.Internal)] (c) =>
    {
      if ((!useExactString || !c.Equals(_lowerCaseName)) && (useExactString || !c.StartsWith(_lowerCaseName)))
        return false;
      Vector2i vector2i2;
      int num;
      return !PrefabManagerStatic.TileMinMaxCounts.TryGetValue(c, out vector2i2) || !this.StreetTilesUsed.TryGetValue(c, out num) || num < vector2i2.y;
    })).OrderByDescending<string, float>((Func<string, float>) ([PublicizedFrom(EAccessModifier.Internal)] (c) => (PrefabManagerStatic.TileMinMaxCounts.TryGetValue(c, out vector2i1) ? (float) vector2i1.x : 0.0f) + rnd.RandomRange(0.0f, 1f))).FirstOrDefault<string>();
    GameRandomManager.Instance.FreeGameRandom(rnd);
    if (key != null)
      return this.prefabManagerData.AllPrefabDatas[key];
    Log.Warning($"Tile starting with {_lowerCaseName} not found!");
    return (PrefabData) null;
  }

  public bool SavePrefabData(Stream _stream)
  {
    try
    {
      XmlDocument xmlDocument = new XmlDocument();
      xmlDocument.CreateXmlDeclaration();
      XmlElement _node = xmlDocument.AddXmlElement("prefabs");
      for (int index = 0; index < this.UsedPrefabsWorld.Count; ++index)
      {
        PrefabDataInstance prefabDataInstance = this.UsedPrefabsWorld[index];
        if (prefabDataInstance != null)
        {
          string str = "";
          if (prefabDataInstance.prefab != null && prefabDataInstance.prefab.location.Type != PathAbstractions.EAbstractedLocationType.None)
            str = prefabDataInstance.prefab.location.Name;
          else if (prefabDataInstance.location.Type != PathAbstractions.EAbstractedLocationType.None)
            str = prefabDataInstance.location.Name;
          _node.AddXmlElement("decoration").SetAttrib("type", "model").SetAttrib("name", str).SetAttrib("position", prefabDataInstance.boundingBoxPosition.ToStringNoBlanks()).SetAttrib("rotation", prefabDataInstance.rotation.ToString());
        }
      }
      xmlDocument.Save(_stream);
      return true;
    }
    catch (Exception ex)
    {
      Log.Exception(ex);
      return false;
    }
  }

  public void GetPrefabsAround(
    Vector3 _position,
    float _distance,
    Dictionary<int, PrefabDataInstance> _prefabs)
  {
    for (int index = 0; index < this.UsedPrefabsWorld.Count; ++index)
    {
      PrefabDataInstance prefabDataInstance = this.UsedPrefabsWorld[index];
      _prefabs[this.UsedPrefabsWorld[index].id] = this.UsedPrefabsWorld[index];
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public float getScoreForPrefab(PrefabData prefab, Vector2i center)
  {
    float num1 = 1f;
    float num2 = 1f;
    FastTags<TagGroup.Poi> _other = FastTags<TagGroup.Poi>.Parse(this.worldBuilder.GetBiome(center).ToString());
    PrefabManager.POIWeightData poiWeightData1 = (PrefabManager.POIWeightData) null;
    for (int index = 0; index < PrefabManagerStatic.prefabWeightData.Count; ++index)
    {
      PrefabManager.POIWeightData poiWeightData2 = PrefabManagerStatic.prefabWeightData[index];
      bool flag = poiWeightData2.PartialPOIName.Length > 0 && prefab.Name.Contains(poiWeightData2.PartialPOIName, StringComparison.OrdinalIgnoreCase);
      if (flag && !poiWeightData2.BiomeTags.IsEmpty && !poiWeightData2.BiomeTags.Test_AnySet(_other))
        return float.MinValue;
      if (flag || !poiWeightData2.Tags.IsEmpty && (!prefab.Tags.IsEmpty && prefab.Tags.Test_AnySet(poiWeightData2.Tags) || !prefab.ThemeTags.IsEmpty && prefab.ThemeTags.Test_AnySet(poiWeightData2.Tags)))
      {
        poiWeightData1 = poiWeightData2;
        break;
      }
    }
    if (poiWeightData1 != null)
    {
      num2 = poiWeightData1.Weight;
      num1 += poiWeightData1.Bias;
      int num3;
      int num4 = this.WorldUsedPrefabNames.TryGetValue(prefab.Name, out num3) ? num3 : 0;
      if (num4 < poiWeightData1.MinCount)
        num1 += (float) (poiWeightData1.MinCount - num4);
      int num5;
      if (this.WorldUsedPrefabNames.TryGetValue(prefab.Name, out num5) && num5 >= poiWeightData1.MaxCount)
        num2 = 0.0f;
    }
    float num6 = num1 + (float) prefab.DifficultyTier / 5f;
    int num7;
    if (this.WorldUsedPrefabNames.TryGetValue(prefab.Name, out num7))
      num6 /= (float) num7 + 1f;
    return num6 * num2;
  }

  public void AddUsedPrefab(string prefabName)
  {
    int num;
    if (this.WorldUsedPrefabNames.TryGetValue(prefabName, out num))
      this.WorldUsedPrefabNames[prefabName] = num + 1;
    else
      this.WorldUsedPrefabNames.Add(prefabName, 1);
  }

  public void AddUsedPrefabWorld(int townshipID, PrefabDataInstance pdi)
  {
    this.UsedPrefabsWorld.Add(pdi);
    this.AddUsedPrefab(pdi.prefab.Name);
  }

  public class POIWeightData
  {
    public string PartialPOIName;
    public FastTags<TagGroup.Poi> Tags;
    public FastTags<TagGroup.Poi> BiomeTags;
    public float Weight;
    public float Bias;
    public int MinCount;
    public int MaxCount;

    public POIWeightData(
      string _partialPOIName,
      FastTags<TagGroup.Poi> _tags,
      FastTags<TagGroup.Poi> _biomeTags,
      float _weight,
      float _bias,
      int minCount,
      int maxCount)
    {
      this.PartialPOIName = _partialPOIName.ToLower();
      this.Tags = _tags;
      this.BiomeTags = _biomeTags;
      this.Weight = _weight;
      this.Bias = _bias;
      this.MinCount = minCount;
      this.MaxCount = maxCount;
    }
  }
}
