// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.DistrictPlanner
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using System.Collections.Generic;
using UniLinq;
using UnityEngine;

#nullable disable
namespace WorldGenerationEngineFinal;

public class DistrictPlanner
{
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly WorldBuilder worldBuilder;
  public Dictionary<string, DynamicProperties> Properties = new Dictionary<string, DynamicProperties>();
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly List<DistrictPlanner.SortingGroup> districtGroups = new List<DistrictPlanner.SortingGroup>();
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly Dictionary<Vector2i, int> groups = new Dictionary<Vector2i, int>();
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly Dictionary<string, DistrictPlanner.SortingGroup> biggestDistrictGroups = new Dictionary<string, DistrictPlanner.SortingGroup>();
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly List<Vector2i> directionsRnd = new List<Vector2i>();
  [PublicizedFrom(EAccessModifier.Private)]
  public Vector2i[] directions4 = new Vector2i[4]
  {
    new Vector2i(0, 1),
    new Vector2i(1, 0),
    new Vector2i(0, -1),
    new Vector2i(-1, 0)
  };

  public DistrictPlanner(WorldBuilder _worldBuilder) => this.worldBuilder = _worldBuilder;

  public void PlanTownship(Township _township)
  {
    if (!_township.IsRoadside())
      this.generateDistricts(_township);
    if (!_township.Data.SpawnGateway)
      return;
    if (_township.IsRoadside())
    {
      this.GenerateGateway(_township);
    }
    else
    {
      int num = _township.IsBig() ? 2 : 1;
      for (int index = 0; index < num; ++index)
      {
        foreach (Vector2i _direction in this.directions4)
          this.GenerateGatewayDir(_township, _direction);
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void generateDistricts(Township _township)
  {
    if (_township.Streets.Count == 0)
      return;
    GameRandom gameRandom = GameRandomManager.Instance.CreateGameRandom(this.worldBuilder.Seed + _township.GridCenter.x + _township.GridCenter.y);
    bool flag1 = _township.CalcCenterStreetTile().BiomeType == BiomeType.wasteland;
    Dictionary<string, District> dictionary = new Dictionary<string, District>();
    float num1 = 0.0f;
    string lower = _township.GetTypeName().ToLower();
    foreach (KeyValuePair<string, District> district1 in DistrictPlannerStatic.Districts)
    {
      string str;
      District district2;
      district1.Deconstruct(ref str, ref district2);
      string key = str;
      District _other = district2;
      if ((flag1 || !key.Contains("wasteland")) && _other.townships.Test_AnySet(FastTags<TagGroup.Poi>.Parse(lower)) && (double) _other.weight != 0.0)
      {
        dictionary.Add(key, new District(_other));
        num1 += _other.weight;
      }
    }
    foreach (KeyValuePair<string, District> keyValuePair in dictionary)
    {
      District district3 = keyValuePair.Value;
      district3.weight /= num1;
      foreach (string neighborDistrict in district3.avoidedNeighborDistricts)
      {
        District district4;
        if (dictionary.TryGetValue(neighborDistrict, out district4) && !district4.avoidedNeighborDistricts.Contains(keyValuePair.Key))
          district4.avoidedNeighborDistricts.Add(keyValuePair.Key);
      }
    }
    List<string> list1 = dictionary.OrderBy<KeyValuePair<string, District>, float>((Func<KeyValuePair<string, District>, float>) ([PublicizedFrom(EAccessModifier.Internal)] (entry) => !(entry.Value.name == "downtown") ? entry.Value.weight : 0.0f)).Select<KeyValuePair<string, District>, string>((Func<KeyValuePair<string, District>, string>) ([PublicizedFrom(EAccessModifier.Internal)] (entry) => entry.Key)).ToList<string>();
    List<StreetTile> list2 = new List<StreetTile>();
    foreach (StreetTile streetTile in _township.Streets.Values)
    {
      if (streetTile.District != null)
        ++streetTile.District.counter;
      else
        list2.Add(streetTile);
    }
    DistrictPlanner.Shuffle<StreetTile>(this.worldBuilder.Seed + _township.GridCenter.x + _township.GridCenter.y, list2);
    foreach (string key in list1)
    {
      District district = dictionary[key];
      int num2 = Mathf.CeilToInt((float) list2.Count * district.weight);
      foreach (StreetTile streetTile in list2)
      {
        if (streetTile.District == null)
        {
          if (district.counter < num2)
          {
            bool flag2 = true;
            foreach (StreetTile neighbor in streetTile.GetNeighbors())
            {
              if (neighbor.District != null && district.avoidedNeighborDistricts.Contains(neighbor.District.name))
              {
                flag2 = false;
                break;
              }
            }
            if (flag2)
            {
              ++district.counter;
              streetTile.District = district;
              streetTile.Used = true;
              streetTile.SetPathingConstraintsForTile(true);
            }
          }
          else
            break;
        }
      }
    }
    foreach (StreetTile streetTile in list2)
    {
      if (streetTile.District == null)
      {
        streetTile.Township = (Township) null;
        _township.Streets.Remove(streetTile.GridPosition);
      }
    }
    this.GroupDistricts(_township, dictionary);
    GameRandomManager.Instance.FreeGameRandom(gameRandom);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void GenerateGateway(Township _township)
  {
    foreach (StreetTile streetTile in _township.Streets.Values)
    {
      streetTile.District = DistrictPlannerStatic.Districts["gateway"];
      streetTile.Township = _township;
      _township.Gateways.Add(streetTile);
      streetTile.SetPathingConstraintsForTile(true);
      streetTile.SetRoadExits(true, true, true, true);
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void GenerateGatewayDir(Township _township, Vector2i _direction)
  {
    Vector2i direction1 = _direction;
    foreach (StreetTile streetTile in _township.Streets.Values)
    {
      if (streetTile.District == null || !(streetTile.District.name == "gateway"))
      {
        StreetTile neighbor1 = streetTile.GetNeighbor(direction1);
        if (!_township.Streets.ContainsKey(neighbor1.GridPosition) && neighbor1.IsValidForGateway)
        {
          int num1 = 0;
          int num2 = 0;
          int index = -1;
          bool[] _exits = new bool[4];
          foreach (Vector2i direction2 in this.directions4)
          {
            ++index;
            StreetTile neighbor2 = neighbor1.GetNeighbor(direction2);
            if (neighbor2 != null && neighbor2.IsValidForGateway)
            {
              _exits[index] = true;
              ++num2;
              if (neighbor2.Township != null)
              {
                ++num1;
                if (num1 > 1)
                  break;
              }
            }
          }
          if (num1 == 1 && num2 >= 2)
          {
            neighbor1.District = DistrictPlannerStatic.Districts["gateway"];
            neighbor1.Township = _township;
            neighbor1.Used = true;
            neighbor1.SetRoadExits(_exits);
            neighbor1.SetPathingConstraintsForTile(true);
            foreach (StreetTile neighbor3 in neighbor1.GetNeighbors())
              neighbor3.SetPathingConstraintsForTile();
            _township.Streets.Add(neighbor1.GridPosition, neighbor1);
            _township.Gateways.Add(neighbor1);
            break;
          }
        }
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static void Shuffle<T>(int seed, List<T> list)
  {
    int count = list.Count;
    GameRandom gameRandom = GameRandomManager.Instance.CreateGameRandom(seed);
    while (count > 1)
    {
      --count;
      int index1 = gameRandom.RandomRange(0, count) % count;
      List<T> objList1 = list;
      int num = index1;
      List<T> objList2 = list;
      int index2 = count;
      T obj1 = list[count];
      T obj2 = list[index1];
      int index3 = num;
      T obj3;
      T obj4 = obj3 = obj1;
      objList1[index3] = obj3;
      objList2[index2] = obj4 = obj2;
    }
    GameRandomManager.Instance.FreeGameRandom(gameRandom);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void GroupDistricts(Township _township, Dictionary<string, District> districtList)
  {
    for (int index1 = 0; index1 < 100; ++index1)
    {
      this.districtGroups.Clear();
      this.groups.Clear();
      foreach (Vector2i key1 in _township.Streets.Keys)
      {
        District district = _township.Streets[key1].District;
        int index2 = this.districtGroups.Count;
        for (int index3 = 0; index3 < this.directions4.Length; ++index3)
        {
          Vector2i key2 = key1 + this.directions4[index3];
          StreetTile streetTile;
          int num;
          if (_township.Streets.TryGetValue(key2, out streetTile) && streetTile.District == district && this.groups.TryGetValue(key2, out num))
            index2 = num;
        }
        this.groups[key1] = index2;
        if (index2 == this.districtGroups.Count)
          this.districtGroups.Add(new DistrictPlanner.SortingGroup()
          {
            District = district
          });
        this.districtGroups[index2].Positions.Add(key1);
      }
      if (this.districtGroups.Count > districtList.Count)
      {
        this.biggestDistrictGroups.Clear();
        string str;
        District district1;
        foreach (KeyValuePair<string, District> district2 in districtList)
        {
          district2.Deconstruct(ref str, ref district1);
          string key = str;
          District district3 = district1;
          int num = int.MinValue;
          DistrictPlanner.SortingGroup sortingGroup = (DistrictPlanner.SortingGroup) null;
          for (int index4 = 0; index4 < this.districtGroups.Count; ++index4)
          {
            DistrictPlanner.SortingGroup districtGroup = this.districtGroups[index4];
            if (districtGroup.District == district3 && districtGroup.Positions.Count > num)
            {
              num = districtGroup.Positions.Count;
              sortingGroup = districtGroup;
            }
          }
          if (sortingGroup != null)
            this.biggestDistrictGroups.Add(key, sortingGroup);
        }
        foreach (KeyValuePair<string, District> district4 in districtList)
        {
          district4.Deconstruct(ref str, ref district1);
          string key3 = str;
          District value = district1;
          List<DistrictPlanner.SortingGroup> all = this.districtGroups.FindAll((Predicate<DistrictPlanner.SortingGroup>) ([PublicizedFrom(EAccessModifier.Internal)] (_group) => _group.District == value));
          if (all.Count != 0)
          {
            all.Sort((Comparison<DistrictPlanner.SortingGroup>) ([PublicizedFrom(EAccessModifier.Internal)] (_groupA, _groupB) => _groupB.Positions.Count.CompareTo(_groupA.Positions.Count)));
            DistrictPlanner.SortingGroup biggestDistrictGroup = this.biggestDistrictGroups[key3];
            for (int index5 = 0; index5 < all.Count; ++index5)
            {
              if (biggestDistrictGroup != all[index5])
              {
                for (int index6 = 0; index6 < all[index5].Positions.Count; ++index6)
                {
                  Vector2i position1 = all[index5].Positions[index6];
                  if (_township.Streets[position1].District == value)
                  {
                    for (int index7 = 0; index7 < this.directions4.Length; ++index7)
                    {
                      Vector2i key4 = position1 + this.directions4[index7];
                      Vector2i vector2i1 = new Vector2i();
                      StreetTile streetTile1;
                      if (_township.Streets.TryGetValue(key4, out streetTile1))
                      {
                        District district5 = streetTile1.District;
                        if (district5.type != District.Type.Downtown && district5.type != District.Type.Gateway && district5.type != District.Type.Rural)
                        {
                          int hashCode = key4.ToString().GetHashCode();
                          DistrictPlanner.Shuffle<Vector2i>(hashCode, biggestDistrictGroup.Positions);
                          this.directionsRnd.Clear();
                          this.directionsRnd.AddRange((IEnumerable<Vector2i>) this.directions4);
                          DistrictPlanner.Shuffle<Vector2i>(hashCode, this.directionsRnd);
                          foreach (Vector2i position2 in biggestDistrictGroup.Positions)
                          {
                            foreach (Vector2i vector2i2 in this.directionsRnd)
                            {
                              Vector2i key5 = position2 + vector2i2;
                              StreetTile streetTile2;
                              if (_township.Streets.TryGetValue(key5, out streetTile2) && streetTile2.District == district5)
                              {
                                bool flag = true;
                                foreach (StreetTile neighbor in streetTile2.GetNeighbors())
                                {
                                  if (neighbor.District != null && value.avoidedNeighborDistricts.Contains(neighbor.District.name))
                                  {
                                    flag = false;
                                    break;
                                  }
                                }
                                if (flag)
                                {
                                  streetTile2.District = value;
                                  _township.Streets[position1].District = district5;
                                  vector2i1 = key5;
                                  break;
                                }
                              }
                            }
                            if (vector2i1 != new Vector2i())
                              break;
                          }
                          if (vector2i1 != new Vector2i())
                          {
                            biggestDistrictGroup.Positions.Add(vector2i1);
                            break;
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
      else
        break;
    }
    this.districtGroups.Clear();
    this.groups.Clear();
    this.biggestDistrictGroups.Clear();
    this.directionsRnd.Clear();
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public class SortingGroup
  {
    public District District;
    public List<Vector2i> Positions = new List<Vector2i>();
  }
}
