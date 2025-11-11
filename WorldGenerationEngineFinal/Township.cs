// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.Township
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using UnityEngine;

#nullable disable
namespace WorldGenerationEngineFinal;

public class Township
{
  [PublicizedFrom(EAccessModifier.Private)]
  public const int BUFFER_DISTANCE = 300;
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly WorldBuilder worldBuilder;
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly TownshipData townshipData;
  public int ID;
  public BiomeType BiomeType;
  public Rect Area;
  public Rect BufferArea;
  [PublicizedFrom(EAccessModifier.Private)]
  public StreetTile commercialCap;
  [PublicizedFrom(EAccessModifier.Private)]
  public StreetTile ruralCap;
  public Vector2i GridCenter;
  public int Height;
  [PublicizedFrom(EAccessModifier.Private)]
  public StreetTile centerMostTile;
  public Dictionary<Vector2i, StreetTile> Streets = new Dictionary<Vector2i, StreetTile>();
  public List<PrefabDataInstance> Prefabs = new List<PrefabDataInstance>();
  public List<StreetTile> Gateways = new List<StreetTile>();
  public Dictionary<Township, int> TownshipConnectionCounts = new Dictionary<Township, int>();
  public GameRandom rand;
  [PublicizedFrom(EAccessModifier.Private)]
  public List<Vector2i> list = new List<Vector2i>();

  public Township(WorldBuilder _worldBuilder, TownshipData _townshipData)
  {
    this.worldBuilder = _worldBuilder;
    this.townshipData = _townshipData;
  }

  public void Cleanup() => GameRandomManager.Instance.FreeGameRandom(this.rand);

  public TownshipData Data => this.townshipData;

  public string GetTypeName() => this.townshipData.Name;

  public bool IsBig() => this.townshipData.Name == "citybig";

  public bool IsRoadside() => this.townshipData.Category == TownshipData.eCategory.Roadside;

  public bool IsRural() => this.townshipData.Category == TownshipData.eCategory.Rural;

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public bool IsWilderness() => this.townshipData.Category == TownshipData.eCategory.Wilderness;

  public void SortGatewaysClockwise()
  {
    this.Gateways.Sort((Comparison<StreetTile>) ([PublicizedFrom(EAccessModifier.Private)] (_t1, _t2) => Mathf.Atan2((float) (_t1.GridPosition.y - this.GridCenter.y), (float) (_t1.GridPosition.x - this.GridCenter.x)).CompareTo(Mathf.Atan2((float) (_t2.GridPosition.y - this.GridCenter.y), (float) (_t2.GridPosition.x - this.GridCenter.x)))));
  }

  public void CleanupStreets()
  {
    if (this.Streets == null || this.Streets.Count == 0)
    {
      Log.Error("No Streets!");
    }
    else
    {
      this.rand = GameRandomManager.Instance.CreateGameRandom(this.worldBuilder.Seed + this.ID + this.Streets.Count);
      foreach (StreetTile streetTile in this.Streets.Values)
      {
        if (streetTile.District != null && streetTile.District.type != District.Type.Gateway)
        {
          int num = 0;
          if (this.commercialCap == null && streetTile.District.type == District.Type.Commercial)
          {
            for (int index = 0; index < this.worldBuilder.TownshipShared.dir4way.Length; ++index)
            {
              StreetTile neighbor = streetTile.GetNeighbor(this.worldBuilder.TownshipShared.dir4way[index]);
              if (neighbor != null && neighbor.District == streetTile.District)
                ++num;
              if (neighbor != null && neighbor == this.ruralCap)
                num += 2;
            }
            if (num == 1)
            {
              for (int index = 0; index < this.worldBuilder.TownshipShared.dir4way.Length; ++index)
              {
                StreetTile neighbor = streetTile.GetNeighbor(this.worldBuilder.TownshipShared.dir4way[index]);
                if (neighbor != null && neighbor.District == streetTile.District)
                  streetTile.SetExitUsed(streetTile.getHighwayExitPosition(index));
                else
                  streetTile.SetExitUnUsed(streetTile.getHighwayExitPosition(index));
              }
              this.commercialCap = streetTile;
            }
          }
          else if (this.ruralCap == null && streetTile.District.type == District.Type.Rural)
          {
            for (int index = 0; index < this.worldBuilder.TownshipShared.dir4way.Length; ++index)
            {
              StreetTile neighbor = streetTile.GetNeighbor(this.worldBuilder.TownshipShared.dir4way[index]);
              if (neighbor != null && neighbor.District == streetTile.District)
                ++num;
              if (neighbor != null && neighbor == this.commercialCap)
                num += 2;
            }
            if (num >= 1 && num <= 2)
            {
              bool flag = false;
              for (int index = 0; index < this.worldBuilder.TownshipShared.dir4way.Length; ++index)
              {
                StreetTile neighbor = streetTile.GetNeighbor(this.worldBuilder.TownshipShared.dir4way[index]);
                if (!flag && neighbor != null && neighbor.District == streetTile.District)
                {
                  streetTile.SetExitUsed(streetTile.getHighwayExitPosition(index));
                  flag = true;
                }
                else
                  streetTile.SetExitUnUsed(streetTile.getHighwayExitPosition(index));
              }
              this.ruralCap = streetTile;
            }
          }
          else
          {
            for (int index = 0; index < this.worldBuilder.TownshipShared.dir4way.Length; ++index)
            {
              StreetTile neighbor = streetTile.GetNeighbor(this.worldBuilder.TownshipShared.dir4way[index]);
              if (neighbor != null && neighbor.District != null && neighbor.District.type == District.Type.Gateway)
                ++num;
            }
            if (num >= 1)
            {
              for (int index = 0; index < this.worldBuilder.TownshipShared.dir4way.Length; ++index)
              {
                StreetTile neighbor = streetTile.GetNeighbor(this.worldBuilder.TownshipShared.dir4way[index]);
                if (neighbor != null && (neighbor.District == streetTile.District || neighbor.District == DistrictPlannerStatic.Districts["gateway"]))
                  streetTile.SetExitUsed(streetTile.getHighwayExitPosition(index));
              }
            }
          }
        }
      }
      this.cleanupLessThan();
      this.cleanupGreaterThan();
      this.cleanupNotEqual();
      this.cleanupLessThan();
      this.cleanupGreaterThan();
      this.cleanupNotEqual();
      int num1 = int.MaxValue;
      int num2 = int.MaxValue;
      int v1_1 = int.MinValue;
      int v1_2 = int.MinValue;
      foreach (StreetTile streetTile in this.Streets.Values)
      {
        num1 = Utils.FastMin(num1, streetTile.WorldPosition.x);
        num2 = Utils.FastMin(num2, streetTile.WorldPosition.y);
        v1_1 = Utils.FastMax(v1_1, streetTile.WorldPositionMax.x);
        v1_2 = Utils.FastMax(v1_2, streetTile.WorldPositionMax.y);
      }
      this.Area = new Rect((float) num1, (float) num2, (float) (v1_1 - num1), (float) (v1_2 - num2));
      this.BufferArea = new Rect(this.Area.xMin - 150f, this.Area.yMin - 150f, this.Area.width + 300f, this.Area.height + 300f);
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void cleanupLessThan()
  {
    foreach (StreetTile streetTile in this.Streets.Values)
    {
      int roadExitCount = streetTile.RoadExitCount;
      int neighborExitCount = this.GetNeighborExitCount(streetTile);
      if (!(streetTile.District.name == "gateway") && streetTile != this.ruralCap && streetTile != this.commercialCap && roadExitCount < neighborExitCount)
      {
        for (int index1 = 0; index1 < this.worldBuilder.StreetTileShared.RoadShapeExitCounts.Count; ++index1)
        {
          if (this.worldBuilder.StreetTileShared.RoadShapeExitCounts[index1] == neighborExitCount)
          {
            for (int index2 = 0; index2 < this.worldBuilder.TownshipShared.dir4way.Length; ++index2)
            {
              StreetTile neighbor = streetTile.GetNeighbor(this.worldBuilder.TownshipShared.dir4way[index2]);
              if (neighbor.Township != streetTile.Township || !neighbor.HasExitTo(streetTile))
                streetTile.SetExitUnUsed(streetTile.getHighwayExitPosition(index2));
              else
                streetTile.SetExitUsed(streetTile.getHighwayExitPosition(index2));
            }
          }
        }
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void cleanupGreaterThan()
  {
    foreach (StreetTile streetTile in this.Streets.Values)
    {
      int roadExitCount = streetTile.RoadExitCount;
      int neighborExitCount = this.GetNeighborExitCount(streetTile);
      if (!(streetTile.District.name == "gateway") && streetTile != this.ruralCap && streetTile != this.commercialCap && roadExitCount > neighborExitCount)
      {
        for (int index1 = 0; index1 < this.worldBuilder.StreetTileShared.RoadShapeExitCounts.Count; ++index1)
        {
          if (this.worldBuilder.StreetTileShared.RoadShapeExitCounts[index1] == neighborExitCount)
          {
            for (int index2 = 0; index2 < this.worldBuilder.TownshipShared.dir4way.Length; ++index2)
            {
              StreetTile neighbor = streetTile.GetNeighbor(this.worldBuilder.TownshipShared.dir4way[index2]);
              if (neighbor.Township != streetTile.Township || !neighbor.HasExitTo(streetTile))
                streetTile.SetExitUnUsed(streetTile.getHighwayExitPosition(index2));
              else
                streetTile.SetExitUsed(streetTile.getHighwayExitPosition(index2));
            }
          }
        }
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void cleanupNotEqual()
  {
    foreach (StreetTile streetTile in this.Streets.Values)
    {
      int roadExitCount = streetTile.RoadExitCount;
      int neighborExitCount = this.GetNeighborExitCount(streetTile);
      if (!(streetTile.District.name == "gateway") && streetTile != this.ruralCap && streetTile != this.commercialCap && roadExitCount != neighborExitCount)
      {
        for (int index1 = 0; index1 < this.worldBuilder.StreetTileShared.RoadShapeExitCounts.Count; ++index1)
        {
          if (this.worldBuilder.StreetTileShared.RoadShapeExitCounts[index1] == neighborExitCount)
          {
            for (int index2 = 0; index2 < this.worldBuilder.TownshipShared.dir4way.Length; ++index2)
            {
              StreetTile neighbor = streetTile.GetNeighbor(this.worldBuilder.TownshipShared.dir4way[index2]);
              if (neighbor.Township != streetTile.Township || !neighbor.HasExitTo(streetTile))
                streetTile.SetExitUnUsed(streetTile.getHighwayExitPosition(index2));
              else
                streetTile.SetExitUsed(streetTile.getHighwayExitPosition(index2));
            }
          }
        }
      }
    }
  }

  public void SpawnPrefabs()
  {
    foreach (StreetTile streetTile in this.Streets.Values)
    {
      if (streetTile == null)
        Log.Error("WorldTileData is null, this shouldn't happen!");
      else
        streetTile.SpawnPrefabs();
    }
    this.Prefabs.Clear();
  }

  public StreetTile CalcCenterStreetTile()
  {
    if (this.centerMostTile != null)
      return this.centerMostTile;
    Vector2i zero = Vector2i.zero;
    foreach (StreetTile streetTile in this.Streets.Values)
      zero += streetTile.GridPosition;
    Vector2i a = zero / this.Streets.Count;
    int num1 = int.MaxValue;
    StreetTile streetTile1 = (StreetTile) null;
    foreach (StreetTile streetTile2 in this.Streets.Values)
    {
      int num2 = Vector2i.DistanceSqrInt(a, streetTile2.GridPosition);
      if (num2 < num1)
      {
        num1 = num2;
        streetTile1 = streetTile2;
      }
    }
    this.centerMostTile = streetTile1;
    float num3 = 0.0f;
    Vector2i worldPositionCenter = this.centerMostTile.WorldPositionCenter;
    for (int index1 = -50; index1 <= 50; index1 += 50)
    {
      Vector2i pos;
      pos.y = worldPositionCenter.y + index1;
      for (int index2 = -50; index2 <= 50; index2 += 50)
      {
        pos.x = worldPositionCenter.x + index2;
        num3 += this.worldBuilder.GetHeight(pos);
      }
    }
    this.Height = Mathf.CeilToInt(num3 / 9f);
    this.Height += 3;
    if (this.Streets.Count > 2 && this.Height < 130)
    {
      if (this.BiomeType == BiomeType.snow)
        this.Height += 25;
      else if (this.BiomeType == BiomeType.wasteland)
        this.Height += 12;
    }
    return streetTile1;
  }

  public void AddToUsedPOIList(string name) => this.worldBuilder.PrefabManager.AddUsedPrefab(name);

  public List<Vector2i> GetUnusedTownExits(int _gatewayUnusedMax = 4)
  {
    this.list.Clear();
    if (this.townshipData.SpawnGateway)
    {
      foreach (StreetTile gateway in this.Gateways)
      {
        if (gateway.UsedExitList.Count <= _gatewayUnusedMax)
        {
          foreach (Vector2i highwayExit in gateway.GetHighwayExits(true))
            this.list.Add(highwayExit);
        }
      }
    }
    else
    {
      foreach (StreetTile streetTile in this.Streets.Values)
      {
        foreach (Vector2i highwayExit in streetTile.GetHighwayExits())
          this.list.Add(highwayExit);
      }
    }
    return this.list;
  }

  public void AddPrefab(PrefabDataInstance pdi)
  {
    this.Prefabs.Add(pdi);
    this.worldBuilder.PrefabManager.AddUsedPrefabWorld(this.ID, pdi);
  }

  public List<Vector2i> GetTownExits()
  {
    this.list.Clear();
    foreach (StreetTile gateway in this.Gateways)
    {
      foreach (Vector2i highwayExit in gateway.GetHighwayExits(true))
        this.list.Add(highwayExit);
    }
    return this.list;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public int GetNeighborExitCount(StreetTile current)
  {
    int neighborExitCount = 0;
    int num = -1;
    foreach (StreetTile neighbor in current.GetNeighbors())
    {
      ++num;
      if (neighbor != null && neighbor.District != null && neighbor.Township != null)
      {
        bool flag1 = current.District.name == "highway";
        bool flag2 = current.District.name == "gateway";
        bool flag3 = neighbor.District.name == "highway";
        bool flag4 = neighbor.District.name == "gateway";
        if ((neighbor.Township == current.Township || (flag1 || flag4 || flag2 || flag3) && (!flag2 || flag3) && (!flag1 || flag4)) && (neighbor.RoadExits[num + 2 & 3] || flag1 & flag3))
          ++neighborExitCount;
      }
    }
    return neighborExitCount;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public bool[] GetNeighborExits(StreetTile current)
  {
    bool[] neighborExits = new bool[4];
    int index = -1;
    foreach (StreetTile neighbor in current.GetNeighbors())
    {
      ++index;
      if (neighbor != null && neighbor.District != null && neighbor.Township != null)
      {
        bool flag1 = current.District.name == "highway";
        bool flag2 = current.District.name == "gateway";
        bool flag3 = neighbor.District.name == "highway";
        bool flag4 = neighbor.District.name == "gateway";
        if ((neighbor.Township == current.Township || (flag1 || flag4 || flag2 || flag3) && (!flag2 || flag3) && (!flag1 || flag4)) && (neighbor.HasExitTo(current) || flag1 & flag3))
          neighborExits[index] = true;
      }
    }
    return neighborExits;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public int GetNeighborCount(Vector2i current)
  {
    int neighborCount = 0;
    for (int index = 0; index < this.worldBuilder.TownshipShared.dir4way.Length; ++index)
    {
      if (this.Streets.ContainsKey(current + this.worldBuilder.TownshipShared.dir4way[index]))
        ++neighborCount;
    }
    return neighborCount;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public int GetCurrentExitCount(Vector2i current)
  {
    int currentExitCount = 0;
    for (int index = 0; index < this.Streets[current].RoadExits.Length; ++index)
    {
      if (this.Streets[current].RoadExits[index])
        ++currentExitCount;
    }
    return currentExitCount;
  }

  public override string ToString()
  {
    return $"Township {this.townshipData.Name}, {this.townshipData.Category}, {this.ID}";
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static bool HasExitWhenRotated(
    int hasThisDirExit,
    StreetTile.PrefabRotations _rots,
    bool[] _exits)
  {
    bool[] flagArray = new bool[4];
    switch (_rots)
    {
      case StreetTile.PrefabRotations.None:
        flagArray[0] = _exits[0];
        flagArray[1] = _exits[1];
        flagArray[2] = _exits[2];
        flagArray[3] = _exits[3];
        break;
      case StreetTile.PrefabRotations.One:
        flagArray[0] = _exits[3];
        flagArray[1] = _exits[0];
        flagArray[2] = _exits[1];
        flagArray[3] = _exits[2];
        break;
      case StreetTile.PrefabRotations.Two:
        flagArray[0] = _exits[2];
        flagArray[1] = _exits[3];
        flagArray[2] = _exits[0];
        flagArray[3] = _exits[1];
        break;
      case StreetTile.PrefabRotations.Three:
        flagArray[0] = _exits[1];
        flagArray[1] = _exits[2];
        flagArray[2] = _exits[3];
        flagArray[3] = _exits[0];
        break;
    }
    return flagArray[hasThisDirExit];
  }

  [CompilerGenerated]
  [PublicizedFrom(EAccessModifier.Private)]
  public int \u003CSortGatewaysClockwise\u003Eb__26_0(StreetTile _t1, StreetTile _t2)
  {
    return Mathf.Atan2((float) (_t1.GridPosition.y - this.GridCenter.y), (float) (_t1.GridPosition.x - this.GridCenter.x)).CompareTo(Mathf.Atan2((float) (_t2.GridPosition.y - this.GridCenter.y), (float) (_t2.GridPosition.x - this.GridCenter.x)));
  }
}
