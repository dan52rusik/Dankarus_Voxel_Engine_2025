// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.TownPlanner
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using System.Collections;
using System.Collections.Generic;
using UniLinq;
using UnityEngine;

#nullable disable
namespace WorldGenerationEngineFinal;

public class TownPlanner
{
  [PublicizedFrom(EAccessModifier.Private)]
  public const int cTriesPerTownshipSpawnInfo = 80 /*0x50*/;
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly WorldBuilder worldBuilder;
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly Dictionary<BiomeType, TownPlanner.BiomeStats> biomeStats = new Dictionary<BiomeType, TownPlanner.BiomeStats>();
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly Vector2i[] dir4way = new Vector2i[4]
  {
    Vector2i.up,
    Vector2i.right,
    Vector2i.down,
    Vector2i.left
  };
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly Vector2i[] dir8way = new Vector2i[8]
  {
    Vector2i.up,
    Vector2i.up + Vector2i.right,
    Vector2i.right,
    Vector2i.right + Vector2i.down,
    Vector2i.down,
    Vector2i.down + Vector2i.left,
    Vector2i.left,
    Vector2i.left + Vector2i.up
  };

  public TownPlanner(WorldBuilder _worldBuilder) => this.worldBuilder = _worldBuilder;

  public IEnumerator Plan(DynamicProperties _properties, int worldSeed)
  {
    MicroStopwatch ms = new MicroStopwatch(true);
    Dictionary<BiomeType, List<Vector2i>> biomeStreetTiles = this.getStreetTilesByBiome();
    Dictionary<int, TownPlanner.TownshipSpawnInfo> townshipSpawnInfos = new Dictionary<int, TownPlanner.TownshipSpawnInfo>();
    this.getTownshipCounts(_properties, townshipSpawnInfos);
    int townID = 0;
    List<int> _dest = new List<int>();
    townshipSpawnInfos.CopyKeysTo<int, TownPlanner.TownshipSpawnInfo>((ICollection<int>) _dest);
    _dest.Sort((Comparison<int>) ([PublicizedFrom(EAccessModifier.Internal)] (key1, key2) => townshipSpawnInfos[key2].max.CompareTo(townshipSpawnInfos[key1].max)));
    int rndAdd = 1982;
    GameRandom rnd = GameRandomManager.Instance.CreateGameRandom();
    foreach (int townshipTypeId in _dest)
    {
      TownshipData townshipData;
      if (WorldBuilderStatic.idToTownshipData.TryGetValue(townshipTypeId, out townshipData))
      {
        string townshipTypeName = townshipData.Name;
        yield return (object) this.worldBuilder.SetMessage(string.Format(Localization.Get("xuiRwgTownPlanning"), Application.isEditor ? (object) townshipTypeName : (object) string.Empty));
        rnd.SetSeed(worldSeed + rndAdd++);
        bool flag1 = townshipData.Category == TownshipData.eCategory.Roadside;
        int num1 = this.worldBuilder.StreetTileMapSize / 10;
        if (flag1)
          num1 = this.worldBuilder.StreetTileMapSize / 8;
        int num2 = this.worldBuilder.StreetTileMap.GetLength(0) - num1;
        int num3 = this.worldBuilder.StreetTileMap.GetLength(1) - num1;
        int num4 = 80 /*0x50*/;
        TownPlanner.TownshipSpawnInfo tsSpawnInfo = townshipSpawnInfos[townshipTypeId];
        BiomeType biomeType1 = BiomeType.none;
        int num5 = 0;
        for (int index1 = 0; index1 < tsSpawnInfo.count; ++index1)
        {
          if (biomeType1 == BiomeType.none)
            biomeType1 = this.getBiomeWithMostAvailableSpace(townshipData);
          List<Vector2i> vector2iList = (List<Vector2i>) null;
          BiomeType biomeType2 = biomeType1;
          for (int index2 = 0; index2 < 5; ++index2)
          {
            List<Vector2i> collection;
            if (biomeStreetTiles.TryGetValue(biomeType1, out collection))
            {
              vector2iList = new List<Vector2i>((IEnumerable<Vector2i>) collection);
              for (int index3 = vector2iList.Count - 1; index3 >= 0; --index3)
              {
                Vector2i position = vector2iList[index3];
                if (position.x <= num1 || position.y <= num1 || position.x >= num2 || position.y >= num3 || this.tooClose(position, flag1, tsSpawnInfo))
                  vector2iList.RemoveAt(index3);
              }
              if (vector2iList.Count > 0)
                break;
            }
            biomeType1 = this.nextBiomeType(biomeType1, townshipData);
            if (biomeType1 == biomeType2)
              break;
          }
          if (vector2iList != null && vector2iList.Count != 0)
          {
            List<StreetTile> streetTileList = new List<StreetTile>();
            List<StreetTile> finalTiles = new List<StreetTile>();
            Township _township = new Township(this.worldBuilder, townshipData)
            {
              BiomeType = biomeType1
            };
            int _min = tsSpawnInfo.min;
            if (tsSpawnInfo.max >= 10)
              _min = (tsSpawnInfo.min + tsSpawnInfo.max) / 2;
            int townSize = Utils.FastMax(1, rnd.RandomRange(_min, tsSpawnInfo.max + 1));
            if (num5 == 0)
              Utils.FastMax(1, _min - 5);
            for (; townSize >= tsSpawnInfo.min; --townSize)
            {
              int index4 = rnd.RandomRange(0, vector2iList.Count);
              int num6 = (double) rnd.RandomFloat < 0.5 ? vector2iList.Count - 1 : 1;
              for (int index5 = 0; index5 < vector2iList.Count; ++index5)
              {
                Vector2i startPosition = vector2iList[index4];
                _township.GridCenter = startPosition;
                index4 = (index4 + num6) % vector2iList.Count;
                for (int index6 = 0; index6 < 12; ++index6)
                {
                  this.getStreetLayout(startPosition, townSize, rnd, flag1, streetTileList);
                  if (streetTileList.Count >= townSize && ((double) townshipData.OutskirtDistrictPercent <= 0.0 || this.grow(townshipData, streetTileList, finalTiles)))
                  {
                    index5 = 999999;
                    townSize = -1;
                    break;
                  }
                }
              }
            }
            if (townSize >= 0)
            {
              if (num4 > 0)
              {
                --num4;
                --index1;
                biomeType1 = this.nextBiomeType(biomeType1, townshipData);
              }
              else
              {
                num4 = 80 /*0x50*/;
                biomeType1 = BiomeType.none;
              }
            }
            else
            {
              if ((double) townshipData.OutskirtDistrictPercent > 0.0)
              {
                string outskirtDistrict = townshipData.OutskirtDistrict;
                foreach (StreetTile streetTile in finalTiles)
                {
                  streetTile.Township = _township;
                  streetTile.District = DistrictPlannerStatic.Districts[outskirtDistrict];
                  _township.Streets[streetTile.GridPosition] = streetTile;
                }
              }
              foreach (StreetTile streetTile in streetTileList)
              {
                streetTile.Township = _township;
                _township.Streets[streetTile.GridPosition] = streetTile;
                streetTile.District = (District) null;
              }
              this.worldBuilder.DistrictPlanner.PlanTownship(_township);
              foreach (StreetTile streetTile in _township.Streets.Values)
              {
                if (streetTile.District.type != District.Type.Gateway && !(streetTile.District.name == "roadside"))
                {
                  foreach (Vector2i vector2i in ((IEnumerable<Vector2i>) this.dir4way).OrderBy<Vector2i, int>((Func<Vector2i, int>) ([PublicizedFrom(EAccessModifier.Internal)] (d2) => rnd.RandomRange(0, 100))).ToList<Vector2i>())
                  {
                    StreetTile neighbor = streetTile.GetNeighbor(vector2i);
                    if (neighbor != null)
                    {
                      bool flag2 = true;
                      if (neighbor.District != null && neighbor.District.type != District.Type.Gateway && neighbor.District != streetTile.District && (double) townshipData.OutskirtDistrictPercent >= 0.60000002384185791)
                        flag2 = (double) rnd.RandomFloat < 0.5;
                      if (neighbor.Township != streetTile.Township)
                        flag2 = false;
                      if (flag2)
                        streetTile.SetExitUsed(streetTile.getHighwayExitPositionByDirection(vector2i));
                      else
                        streetTile.SetExitUnUsed(streetTile.getHighwayExitPositionByDirection(vector2i));
                    }
                  }
                }
              }
              _township.CleanupStreets();
              _township.ID = townID++;
              this.worldBuilder.Townships.Add(_township);
              ++num5;
              TownPlanner.BiomeStats biomeStats;
              if (!this.biomeStats.TryGetValue(biomeType1, out biomeStats))
              {
                biomeStats = new TownPlanner.BiomeStats();
                this.biomeStats.Add(biomeType1, biomeStats);
              }
              ++biomeStats.townshipCount;
              int num7;
              if (!biomeStats.counts.TryGetValue(townshipTypeName, out num7))
                biomeStats.counts.Add(townshipTypeName, 1);
              else
                biomeStats.counts[townshipTypeName] = num7 + 1;
              biomeType1 = this.nextBiomeType(biomeType1, townshipData);
              num4 = 80 /*0x50*/;
            }
          }
          else
            break;
        }
        townshipData = (TownshipData) null;
        townshipTypeName = (string) null;
      }
    }
    Log.Out("TownPlanner Plan {0} in {1}", (object) this.worldBuilder.Townships.Count, (object) (float) ((double) ms.ElapsedMilliseconds * (1.0 / 1000.0)));
    for (int index = 0; index < 5; ++index)
    {
      BiomeType key = (BiomeType) index;
      TownPlanner.BiomeStats biomeStats;
      if (this.biomeStats.TryGetValue(key, out biomeStats))
      {
        string str = "";
        foreach (KeyValuePair<string, int> count in biomeStats.counts)
          str += $", {count.Key} {count.Value}";
        Log.Out("TownPlanner {0} has {1} townships{2}", (object) key, (object) biomeStats.townshipCount, (object) str);
      }
    }
    this.biomeStats.Clear();
    yield return (object) this.worldBuilder.SetMessage(Localization.Get("xuiRwgTownPlanningFinished"));
  }

  public IEnumerator SpawnPrefabs()
  {
    yield return (object) null;
    MicroStopwatch ms = new MicroStopwatch(true);
    MicroStopwatch msReset = new MicroStopwatch(true);
    foreach (Township township in this.worldBuilder.Townships)
    {
      township.SpawnPrefabs();
      if (msReset.ElapsedMilliseconds > 500L)
      {
        yield return (object) null;
        msReset.ResetAndRestart();
      }
    }
    Log.Out($"TownPlanner SpawnPrefabs in {(ValueType) (float) ((double) ms.ElapsedMilliseconds * (1.0 / 1000.0))}, r={Rand.Instance.PeekSample():x}");
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public bool tooClose(
    Vector2i position,
    bool isRoadsideTownship,
    TownPlanner.TownshipSpawnInfo tsSpawnInfo)
  {
    int num1 = isRoadsideTownship ? 3 : tsSpawnInfo.distance;
    int num2 = 5;
    foreach (Township township in this.worldBuilder.Townships)
    {
      bool flag = township.IsRoadside();
      foreach (Vector2i key in township.Streets.Keys)
      {
        if (isRoadsideTownship & flag && (double) Utils.FastAbs((float) (position.x - key.x)) < (double) num2 && (double) Utils.FastAbs((float) (position.y - key.y)) < (double) num2 || (double) Utils.FastAbs((float) (position.x - key.x)) < (double) num1 && (double) Utils.FastAbs((float) (position.y - key.y)) < (double) num1)
          return true;
      }
    }
    return false;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public bool grow(TownshipData _data, List<StreetTile> baseTiles, List<StreetTile> finalTiles)
  {
    finalTiles.Clear();
    Vector2i[] vector2iArray = (double) _data.OutskirtDistrictPercent < 1.0 ? this.dir4way : this.dir8way;
    foreach (StreetTile baseTile in baseTiles)
    {
      foreach (Vector2i vector2i in vector2iArray)
      {
        StreetTile streetTileGrid = this.worldBuilder.GetStreetTileGrid(baseTile.GridPosition + vector2i);
        if (streetTileGrid != null && !baseTiles.Contains(streetTileGrid) && !finalTiles.Contains(streetTileGrid))
        {
          if (!streetTileGrid.IsValidForStreetTile)
            return false;
          foreach (StreetTile neighbor in streetTileGrid.GetNeighbors())
          {
            if (neighbor.Township != null)
              return false;
          }
          finalTiles.Add(streetTileGrid);
        }
      }
    }
    if ((double) _data.OutskirtDistrictPercent < 1.0)
    {
      float num1 = 1f - _data.OutskirtDistrictPercent;
      float num2 = 0.0f;
      for (int index = finalTiles.Count - 1; index >= 0; --index)
      {
        num2 += num1;
        if ((double) num2 >= 1.0)
        {
          --num2;
          finalTiles.RemoveAt(index);
        }
      }
    }
    return true;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public int getTownshipCounts(
    DynamicProperties _properties,
    Dictionary<int, TownPlanner.TownshipSpawnInfo> townshipSpawnInfo)
  {
    int townshipCounts = 0;
    GameRandom gameRandom = Rand.Instance.gameRandom;
    foreach (KeyValuePair<int, TownshipData> keyValuePair in WorldBuilderStatic.idToTownshipData)
    {
      TownshipData townshipData = keyValuePair.Value;
      if (townshipData.Category != TownshipData.eCategory.Wilderness)
      {
        string lower = townshipData.Name.ToLower();
        float optionalValue1 = 1f;
        float optionalValue2 = 1f;
        _properties.ParseVec($"{lower}.tiles", ref optionalValue1, ref optionalValue2);
        int count = this.worldBuilder.GetCount(lower, this.worldBuilder.Towns, gameRandom);
        int optionalValue = 5;
        _properties.ParseInt($"{lower}.distance", ref optionalValue);
        if (count >= 1 && (double) optionalValue1 >= 1.0 && (double) optionalValue2 >= 1.0)
          townshipSpawnInfo.Add(townshipData.Id, new TownPlanner.TownshipSpawnInfo((int) optionalValue1, (int) optionalValue2, count, optionalValue));
        townshipCounts += count;
      }
    }
    return townshipCounts;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public BiomeType nextBiomeType(BiomeType _current, TownshipData _townshipData)
  {
    int index1 = (int) _current;
    for (int index2 = 0; index2 < 4; ++index2)
    {
      index1 = (index1 + 1) % 5;
      if (_townshipData.Biomes.IsEmpty || _townshipData.Biomes.Test_Bit(this.worldBuilder.biomeTagBits[index1]))
        return (BiomeType) index1;
    }
    return BiomeType.none;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public BiomeType getBiomeWithMostAvailableSpace(TownshipData _townshipData)
  {
    int num = 0;
    int mostAvailableSpace = (int) byte.MaxValue;
    List<Vector2i> _biomeStreetTiles = new List<Vector2i>();
    for (int _biomeType = 0; _biomeType < 5; ++_biomeType)
    {
      if (_townshipData.Biomes.IsEmpty || _townshipData.Biomes.Test_Bit(this.worldBuilder.biomeTagBits[_biomeType]))
      {
        this.getStreetTilesForBiome((BiomeType) _biomeType, _biomeStreetTiles);
        if (_biomeStreetTiles.Count > num)
        {
          num = _biomeStreetTiles.Count;
          mostAvailableSpace = _biomeType;
        }
      }
    }
    return (BiomeType) mostAvailableSpace;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public Dictionary<BiomeType, List<Vector2i>> getStreetTilesByBiome()
  {
    Dictionary<BiomeType, List<Vector2i>> streetTilesByBiome = new Dictionary<BiomeType, List<Vector2i>>();
    for (int index = 0; index < 5; ++index)
    {
      List<Vector2i> _biomeStreetTiles = new List<Vector2i>();
      this.getStreetTilesForBiome((BiomeType) index, _biomeStreetTiles);
      if (_biomeStreetTiles.Count > 0)
        streetTilesByBiome.Add((BiomeType) index, _biomeStreetTiles);
    }
    return streetTilesByBiome;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void getStreetTilesForBiome(BiomeType _biomeType, List<Vector2i> _biomeStreetTiles)
  {
    _biomeStreetTiles.Clear();
    StreetTile[,] streetTileMap = this.worldBuilder.StreetTileMap;
    int upperBound1 = streetTileMap.GetUpperBound(0);
    int upperBound2 = streetTileMap.GetUpperBound(1);
    for (int lowerBound1 = streetTileMap.GetLowerBound(0); lowerBound1 <= upperBound1; ++lowerBound1)
    {
      for (int lowerBound2 = streetTileMap.GetLowerBound(1); lowerBound2 <= upperBound2; ++lowerBound2)
      {
        StreetTile streetTile = streetTileMap[lowerBound1, lowerBound2];
        if (streetTile.IsValidForStreetTile && streetTile.Township == null && !streetTile.Used && streetTile.BiomeType == _biomeType)
          _biomeStreetTiles.Add(streetTile.GridPosition);
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public List<TileGroup> getStreetTileGroups()
  {
    List<TileGroup> streetTileGroups = new List<TileGroup>();
    Dictionary<Vector2i, int> dictionary = new Dictionary<Vector2i, int>();
    StreetTile[,] streetTileMap = this.worldBuilder.StreetTileMap;
    int upperBound1 = streetTileMap.GetUpperBound(0);
    int upperBound2 = streetTileMap.GetUpperBound(1);
    for (int lowerBound1 = streetTileMap.GetLowerBound(0); lowerBound1 <= upperBound1; ++lowerBound1)
    {
      for (int lowerBound2 = streetTileMap.GetLowerBound(1); lowerBound2 <= upperBound2; ++lowerBound2)
      {
        StreetTile streetTile = streetTileMap[lowerBound1, lowerBound2];
        int index1 = streetTileGroups.Count;
        if (streetTile.IsValidForStreetTile)
        {
          for (int index2 = 0; index2 < this.dir4way.Length; ++index2)
          {
            if (streetTile.GridPosition.x + this.dir4way[index2].x >= 0 && streetTile.GridPosition.x + this.dir4way[index2].x < this.worldBuilder.StreetTileMap.GetLength(0) && streetTile.GridPosition.y + this.dir4way[index2].y >= 0 && streetTile.GridPosition.y + this.dir4way[index2].y < this.worldBuilder.StreetTileMap.GetLength(1))
            {
              Vector2i key = streetTile.GridPosition + this.dir4way[index2];
              int num;
              if (this.worldBuilder.StreetTileMap[key.x, key.y].BiomeType == streetTile.BiomeType && this.worldBuilder.StreetTileMap[key.x, key.y].IsValidForStreetTile && dictionary.TryGetValue(key, out num))
                index1 = num;
            }
          }
          dictionary[streetTile.GridPosition] = index1;
          if (index1 == streetTileGroups.Count)
            streetTileGroups.Add(new TileGroup()
            {
              Biome = streetTile.BiomeType
            });
          streetTileGroups[index1].Positions.Add(streetTile.GridPosition);
        }
      }
    }
    return streetTileGroups;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void getStreetLayout(
    Vector2i startPosition,
    int townSize,
    GameRandom rnd,
    bool isRoadside,
    List<StreetTile> townTiles)
  {
    townTiles.Clear();
    StreetTile streetTileGrid = this.worldBuilder.GetStreetTileGrid(startPosition);
    if (!isRoadside)
    {
      foreach (Township township in this.worldBuilder.Townships)
      {
        Rect area = streetTileGrid.Area;
        if (area.Overlaps(township.BufferArea))
          return;
        ref Rect local = ref township.BufferArea;
        area = streetTileGrid.Area;
        Vector2 center = area.center;
        if (local.Contains(center) || township.BufferArea.Overlaps(streetTileGrid.Area))
          return;
      }
    }
    else
    {
      if (streetTileGrid.Township != null || streetTileGrid.District != null)
        return;
      foreach (Township township in this.worldBuilder.Townships)
      {
        Rect area = streetTileGrid.Area;
        if (area.Overlaps(township.BufferArea))
          return;
        ref Rect local = ref township.BufferArea;
        area = streetTileGrid.Area;
        Vector2 center = area.center;
        if (local.Contains(center) || township.BufferArea.Overlaps(streetTileGrid.Area))
          return;
      }
      foreach (StreetTile streetTile in streetTileGrid.GetNeighbors8way())
      {
        if (streetTile.Township != null || streetTile.District != null)
          return;
      }
    }
    if (townSize == 1)
    {
      townTiles.Add(streetTileGrid);
    }
    else
    {
      List<StreetTile> streetTileList = new List<StreetTile>();
      if (townSize <= 16 /*0x10*/)
      {
        townTiles.Add(streetTileGrid);
        streetTileList.Add(streetTileGrid);
      }
      else
      {
        int num1 = (int) Mathf.Sqrt((float) townSize) - 1;
        int num2 = num1 / -2;
        int num3 = num1 / 2 - 1;
        for (int index1 = num2; index1 <= num3; ++index1)
        {
          Vector2i _pos;
          _pos.y = startPosition.y + index1;
          bool flag = index1 == num2 || index1 == num3;
          for (int index2 = num2; index2 <= num3; ++index2)
          {
            _pos.x = startPosition.x + index2;
            StreetTile streetTile = this.StreetLayoutCheckPos(_pos);
            if (streetTile == null)
            {
              townTiles.Clear();
              return;
            }
            townTiles.Add(streetTile);
            if (flag || index2 == num2 || index2 == num3)
              streetTileList.Add(streetTile);
          }
        }
        if (townTiles.Count >= townSize)
          return;
      }
      while (streetTileList.Count > 0)
      {
        int index3 = rnd.RandomRange(0, streetTileList.Count);
        StreetTile streetTile1 = streetTileList[index3];
        streetTileList.RemoveAt(index3);
        int num = rnd.RandomRange(4);
        for (int index4 = 0; index4 < 4; ++index4)
        {
          StreetTile streetTile2 = this.StreetLayoutCheckPos(streetTile1.GridPosition + this.dir4way[index4 + num & 3]);
          if (streetTile2 != null && !townTiles.Contains(streetTile2))
          {
            townTiles.Add(streetTile2);
            if (townTiles.Count >= townSize)
              return;
            streetTileList.Add(streetTile2);
          }
        }
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public StreetTile StreetLayoutCheckPos(Vector2i _pos)
  {
    StreetTile streetTileGrid = this.worldBuilder.GetStreetTileGrid(_pos);
    if (streetTileGrid == null || !streetTileGrid.IsValidForStreetTile)
      return (StreetTile) null;
    foreach (Township township in this.worldBuilder.Townships)
    {
      Rect area = streetTileGrid.Area;
      if (!area.Overlaps(township.BufferArea))
      {
        ref Rect local = ref township.BufferArea;
        area = streetTileGrid.Area;
        Vector2 center = area.center;
        if (!local.Contains(center) && !township.BufferArea.Overlaps(streetTileGrid.Area))
          continue;
      }
      return (StreetTile) null;
    }
    return streetTileGrid;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public class BiomeStats
  {
    public int townshipCount;
    public Dictionary<string, int> counts = new Dictionary<string, int>();
  }

  public class TownshipSpawnInfo
  {
    public int min;
    public int max;
    public int count;
    public int distance;

    public TownshipSpawnInfo(int _min, int _max, int _count, int _distance)
    {
      this.min = _min;
      this.max = _max;
      this.count = _count;
      this.distance = _distance;
    }
  }
}
