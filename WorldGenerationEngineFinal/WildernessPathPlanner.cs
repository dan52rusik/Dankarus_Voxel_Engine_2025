// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.WildernessPathPlanner
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

#nullable disable
namespace WorldGenerationEngineFinal;

public class WildernessPathPlanner
{
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly WorldBuilder worldBuilder;

  public WildernessPathPlanner(WorldBuilder _worldBuilder) => this.worldBuilder = _worldBuilder;

  public IEnumerator Plan(int worldSeed)
  {
    MicroStopwatch ms = new MicroStopwatch(true);
    bool hasTowns = this.worldBuilder.highwayPaths.Count > 0;
    List<WorldBuilder.WildernessPathInfo> pathInfos = this.worldBuilder.WildernessPlanner.WildernessPathInfos;
    int n;
    for (n = 0; n < pathInfos.Count; ++n)
    {
      WorldBuilder.WildernessPathInfo wildernessPathInfo = pathInfos[n];
      Vector2 _startPos = wildernessPathInfo.Position.AsVector2();
      float num1 = float.MaxValue;
      Vector2 vector2 = Vector2.zero;
      foreach (Path highwayPath in this.worldBuilder.highwayPaths)
      {
        Vector2 _destPoint;
        if ((double) PathingUtils.FindClosestPathPoint(highwayPath.FinalPathPoints, _startPos, out _destPoint, 3) < 1000000.0)
        {
          int num2 = Utils.FastMin(10, highwayPath.FinalPathPoints.Count - 1);
          for (int index = 0; index < highwayPath.FinalPathPoints.Count; index += num2)
          {
            Vector2 finalPathPoint = highwayPath.FinalPathPoints[index];
            if ((double) (_destPoint - finalPathPoint).sqrMagnitude <= 62500.0)
            {
              Vector2i _end = new Vector2i(finalPathPoint);
              int count = this.worldBuilder.PathingUtils.GetPath(wildernessPathInfo.Position, _end, true).Count;
              if (count >= 2 && (double) count < (double) num1)
              {
                num1 = (float) count;
                vector2 = finalPathPoint;
              }
            }
          }
        }
      }
      wildernessPathInfo.highwayDistance = num1;
      wildernessPathInfo.highwayPoint = vector2;
      if (this.worldBuilder.IsMessageElapsed())
        yield return (object) this.worldBuilder.SetMessage(string.Format(Localization.Get("xuiRwgWildernessPaths"), (object) (n * 50 / pathInfos.Count)));
    }
    pathInfos.Sort((Comparison<WorldBuilder.WildernessPathInfo>) ([PublicizedFrom(EAccessModifier.Internal)] (wp1, wp2) =>
    {
      float num3 = (double) wp1.PathRadius < 2.4000000953674316 ? wp1.PathRadius : 3f;
      float num4 = (double) wp2.PathRadius < 2.4000000953674316 ? wp2.PathRadius : 3f;
      return (double) num3 != (double) num4 ? num4.CompareTo(num3) : wp1.highwayDistance.CompareTo(wp2.highwayDistance);
    }));
    for (n = 0; n < pathInfos.Count; ++n)
    {
      WorldBuilder.WildernessPathInfo wpi = pathInfos[n];
      if (wpi.Path == null)
      {
        Vector2i closestPoint = Vector2i.zero;
        float closestDist = float.MaxValue;
        bool isHighwayConnected = false;
        if ((double) wpi.highwayDistance >= 2.0 && (double) wpi.highwayDistance < 999999.0)
        {
          closestDist = wpi.highwayDistance;
          closestPoint.x = (int) wpi.highwayPoint.x;
          closestPoint.y = (int) wpi.highwayPoint.y;
          isHighwayConnected = true;
        }
        foreach (WorldBuilder.WildernessPathInfo wildernessPathInfo in pathInfos)
        {
          if (wildernessPathInfo != wpi)
          {
            if (wildernessPathInfo.Path == null)
            {
              if (!hasTowns)
              {
                float num = Vector2i.Distance(wpi.Position, wildernessPathInfo.Position);
                if ((double) num < (double) closestDist)
                {
                  closestDist = num;
                  closestPoint = wildernessPathInfo.Position;
                }
              }
            }
            else
            {
              Vector2 _destPoint;
              int _cost;
              if (wildernessPathInfo.Path.connectsToHighway && (double) wildernessPathInfo.PathRadius >= (double) wpi.PathRadius && this.FindShortestPathPointToPathTo(wildernessPathInfo.Path.FinalPathPoints, wpi.Position.AsVector2(), out _destPoint, out _cost))
              {
                float num = (float) _cost;
                if ((double) num < (double) closestDist)
                {
                  closestDist = num;
                  closestPoint.x = (int) _destPoint.x;
                  closestPoint.y = (int) _destPoint.y;
                }
              }
            }
          }
        }
        if (this.worldBuilder.IsMessageElapsed())
          yield return (object) this.worldBuilder.SetMessage(string.Format(Localization.Get("xuiRwgWildernessPaths"), (object) (n * 50 / pathInfos.Count + 50)));
        if ((double) closestDist <= 999999.0)
        {
          Path path = new Path(this.worldBuilder, wpi.Position, closestPoint, wpi.PathRadius, true);
          if (path.IsValid)
          {
            path.connectsToHighway = isHighwayConnected;
            wpi.Path = path;
            this.worldBuilder.wildernessPaths.Add(path);
            this.createTraderSpawnIfAble(path.FinalPathPoints);
          }
          else
            Log.Warning($"WildernessPathPlanner Plan index {n} no path!");
          wpi = (WorldBuilder.WildernessPathInfo) null;
        }
      }
    }
    Log.Out($"WildernessPathPlanner Plan #{pathInfos.Count} in {(ValueType) (float) ((double) ms.ElapsedMilliseconds * (1.0 / 1000.0))}, r={Rand.Instance.PeekSample():x}");
    yield return (object) null;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public bool FindShortestPathPointToPathTo(
    List<Vector2> _path,
    Vector2 _startPos,
    out Vector2 _destPoint,
    out int _cost)
  {
    _destPoint = Vector2.zero;
    _cost = 0;
    if ((double) PathingUtils.FindClosestPathPoint(_path, _startPos, out Vector2 _) < 490000.0)
    {
      Vector2i pathPoint = this.worldBuilder.PathingUtils.GetPathPoint(new Vector2i(_startPos), _path, true, false, out _cost);
      if (_cost > 0)
      {
        _destPoint.x = (float) pathPoint.x;
        _destPoint.y = (float) pathPoint.y;
        return true;
      }
    }
    return false;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void createTraderSpawnIfAble(List<Vector2> pathPoints)
  {
    if (pathPoints.Count < 5)
      return;
    if (this.worldBuilder.ForestBiomeWeight > 0)
    {
      BiomeType biomeType = BiomeType.none;
      for (int index = 2; index < pathPoints.Count - 2; ++index)
      {
        biomeType = this.worldBuilder.GetBiome((int) pathPoints[index].x, (int) pathPoints[index].y);
        if (biomeType == BiomeType.forest)
          break;
      }
      if (biomeType != BiomeType.forest)
        return;
    }
    for (int index = 2; index < pathPoints.Count - 2; ++index)
    {
      if (this.worldBuilder.ForestBiomeWeight <= 0 || this.worldBuilder.GetBiome((int) pathPoints[index].x, (int) pathPoints[index].y) == BiomeType.forest)
      {
        Vector2i vector2i;
        vector2i.x = (int) pathPoints[index].x;
        vector2i.y = (int) pathPoints[index].y;
        StreetTile streetTileWorld = this.worldBuilder.GetStreetTileWorld(vector2i);
        if (streetTileWorld != null && streetTileWorld.HasPrefabs)
        {
          bool flag = true;
          foreach (PrefabDataInstance streetTilePrefabData in streetTileWorld.StreetTilePrefabDatas)
          {
            if (streetTilePrefabData.prefab.DifficultyTier > 1)
            {
              flag = false;
              break;
            }
          }
          if (flag)
            this.worldBuilder.CreatePlayerSpawn(vector2i);
        }
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public int getMaxTraderDistance()
  {
    return (int) (0.10000000149011612 * (double) this.worldBuilder.WorldSize);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public WildernessPathPlanner.WildernessConnectionNode primsAlgo(
    WorldBuilder.WildernessPathInfo startingWildernessPOI,
    bool onlyNonConnected = false)
  {
    List<WorldBuilder.WildernessPathInfo> wildernessPathInfoList = new List<WorldBuilder.WildernessPathInfo>();
    WildernessPathPlanner.WildernessConnectionNode wildernessConnectionNode1 = new WildernessPathPlanner.WildernessConnectionNode(startingWildernessPOI);
    WildernessPathPlanner.WildernessConnectionNode wildernessConnectionNode2 = wildernessConnectionNode1;
    Vector2i _endPosition = Vector2i.zero;
    while (wildernessConnectionNode2 != null)
    {
      int num1 = 262144 /*0x040000*/;
      bool flag = false;
      WorldBuilder.WildernessPathInfo wpi = (WorldBuilder.WildernessPathInfo) null;
      Vector2i position = wildernessConnectionNode2.PathInfo.Position;
      foreach (WorldBuilder.WildernessPathInfo wildernessPathInfo in this.worldBuilder.WildernessPlanner.WildernessPathInfos)
      {
        if (!wildernessPathInfoList.Contains(wildernessPathInfo))
        {
          int num2 = Vector2i.DistanceSqrInt(wildernessPathInfo.Position, position);
          if (num2 < num1 && this.worldBuilder.PathingUtils.GetPathCost(position, wildernessPathInfo.Position, true) > 0)
          {
            _endPosition = wildernessPathInfo.Position;
            num1 = num2;
            wpi = wildernessPathInfo;
            flag = true;
          }
        }
      }
      if (!flag)
      {
        wildernessConnectionNode2 = wildernessConnectionNode2.next;
      }
      else
      {
        wildernessConnectionNode2.Path = new Path(this.worldBuilder, position, _endPosition, wildernessConnectionNode2.PathInfo.PathRadius, true);
        if (!wildernessConnectionNode2.Path.IsValid)
        {
          wildernessConnectionNode2.Path = (Path) null;
          wildernessConnectionNode2 = wildernessConnectionNode2.next;
        }
        else
        {
          wildernessPathInfoList.Add(wpi);
          wildernessConnectionNode2.next = new WildernessPathPlanner.WildernessConnectionNode(wpi);
          wildernessConnectionNode2 = wildernessConnectionNode2.next;
        }
      }
    }
    return wildernessConnectionNode1;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static void Shuffle<T>(int seed, ref List<T> list)
  {
    int count = list.Count;
    GameRandom gameRandom = GameRandomManager.Instance.CreateGameRandom(seed);
    while (count > 1)
    {
      --count;
      int index = gameRandom.RandomRange(0, count) % count;
      T obj = list[index];
      list[index] = list[count];
      list[count] = obj;
    }
    GameRandomManager.Instance.FreeGameRandom(gameRandom);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public class WildernessConnectionNode
  {
    public WildernessPathPlanner.WildernessConnectionNode next;
    public WorldBuilder.WildernessPathInfo PathInfo;
    public Path Path;
    public float Distance;

    public WildernessConnectionNode(WorldBuilder.WildernessPathInfo wpi) => this.PathInfo = wpi;
  }
}
