// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.HighwayPlanner
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

#nullable disable
namespace WorldGenerationEngineFinal;

public class HighwayPlanner
{
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly WorldBuilder worldBuilder;
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly List<HighwayPlanner.ExitConnection> ExitConnections = new List<HighwayPlanner.ExitConnection>();
  [PublicizedFrom(EAccessModifier.Private)]
  public Path getPathToTownshipResult;

  public HighwayPlanner(WorldBuilder _worldBuilder) => this.worldBuilder = _worldBuilder;

  public IEnumerator Plan(DynamicProperties thisWorldProperties, int worldSeed)
  {
    yield return (object) this.worldBuilder.SetMessage(Localization.Get("xuiRwgHighways"));
    MicroStopwatch ms = new MicroStopwatch(true);
    this.ExitConnections.Clear();
    foreach (Township township in this.worldBuilder.Townships)
    {
      foreach (StreetTile gateway in township.Gateways)
        gateway.SetAllExistingNeighborsForGateway();
    }
    List<Township> highwayTownships = this.worldBuilder.Townships.FindAll((Predicate<Township>) ([PublicizedFrom(EAccessModifier.Internal)] (_township) => _township.Data.SpawnGateway));
    if (highwayTownships.Count > 0)
    {
      highwayTownships.Sort((Comparison<Township>) ([PublicizedFrom(EAccessModifier.Internal)] (_t1, _t2) => _t2.Streets.Count.CompareTo(_t1.Streets.Count)));
      for (int n = 0; n < highwayTownships.Count; ++n)
      {
        Township township = highwayTownships[n];
        yield return (object) this.ConnectClosest(township, highwayTownships);
        if (township.IsBig())
          yield return (object) this.ConnectSelf(township);
        township = (Township) null;
      }
      Log.Out($"HighwayPlanner Plan townships in {(ValueType) (float) ((double) ms.ElapsedMilliseconds * (1.0 / 1000.0))}");
      yield return (object) this.cleanupHighwayConnections(highwayTownships);
    }
    yield return (object) this.runTownshipDirtRoads();
    Log.Out($"HighwayPlanner Plan in {(ValueType) (float) ((double) ms.ElapsedMilliseconds * (1.0 / 1000.0))}, r={Rand.Instance.PeekSample():x}");
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public IEnumerator cleanupHighwayConnections(List<Township> highwayTownships)
  {
    MicroStopwatch ms = new MicroStopwatch(true);
    List<Vector2i> tilesToRemove = new List<Vector2i>();
    List<Path> pathsToRemove = new List<Path>();
    foreach (Township highwayTownship in highwayTownships)
    {
      if (highwayTownship.Gateways.Count == 0)
      {
        Log.Error("cleanupHighwayConnections township {0} in {1} has no gateways!", (object) highwayTownship.GetTypeName(), (object) highwayTownship.BiomeType);
      }
      else
      {
        foreach (StreetTile gateway in highwayTownship.Gateways)
        {
          for (int index = 0; index < 4; ++index)
          {
            StreetTile neighborByIndex = gateway.GetNeighborByIndex(index);
            Vector2i highwayExitPosition = gateway.getHighwayExitPosition(index);
            if (!gateway.UsedExitList.Contains(highwayExitPosition) && (neighborByIndex.Township != gateway.Township || !neighborByIndex.HasExitTo(gateway)))
              gateway.SetExitUnUsed(highwayExitPosition);
          }
        }
      }
    }
    foreach (HighwayPlanner.ExitConnection exitConnection in this.ExitConnections)
      exitConnection.SetExitUsedManually();
    foreach (Township highwayTownship in highwayTownships)
    {
      Township t = highwayTownship;
      yield return (object) this.worldBuilder.SetMessage(Localization.Get("xuiRwgHighwaysConnections"));
      if (t.Data.SpawnGateway && t.Gateways.Count != 0)
      {
        foreach (StreetTile gateway in t.Gateways)
        {
          for (int index = 0; index < 4; ++index)
          {
            gateway.GetNeighborByIndex(index);
            Vector2i highwayExitPosition = gateway.getHighwayExitPosition(index);
            if (!gateway.UsedExitList.Contains(highwayExitPosition))
              gateway.SetExitUnUsed(highwayExitPosition);
          }
          if (gateway.UsedExitList.Count < 2)
          {
            for (int index = 0; index < 4; ++index)
            {
              StreetTile neighborByIndex = gateway.GetNeighborByIndex(index);
              gateway.SetExitUnUsed(gateway.getHighwayExitPosition(index));
              if (neighborByIndex.Township == gateway.Township)
                neighborByIndex.SetExitUnUsed(neighborByIndex.getHighwayExitPosition(neighborByIndex.GetNeighborIndex(gateway)));
            }
            foreach (Path connectedHighway in gateway.ConnectedHighways)
            {
              StreetTile streetTileWorld;
              if (this.worldBuilder.GetStreetTileWorld(connectedHighway.StartPosition) == gateway)
              {
                streetTileWorld = this.worldBuilder.GetStreetTileWorld(connectedHighway.EndPosition);
                streetTileWorld.SetExitUnUsed(connectedHighway.EndPosition);
              }
              else
              {
                streetTileWorld = this.worldBuilder.GetStreetTileWorld(connectedHighway.StartPosition);
                streetTileWorld.SetExitUnUsed(connectedHighway.StartPosition);
              }
              if (streetTileWorld.UsedExitList.Count < 2)
                tilesToRemove.Add(streetTileWorld.GridPosition);
              pathsToRemove.Add(connectedHighway);
            }
            tilesToRemove.Add(gateway.GridPosition);
          }
        }
        foreach (Vector2i vector2i in tilesToRemove)
        {
          StreetTile streetTileGrid = this.worldBuilder.GetStreetTileGrid(vector2i);
          if (streetTileGrid.Township != null)
          {
            streetTileGrid.Township.Gateways.Remove(streetTileGrid);
            streetTileGrid.Township.Streets.Remove(vector2i);
          }
          streetTileGrid.StreetTilePrefabDatas.Clear();
          streetTileGrid.District = (District) null;
          streetTileGrid.Township = (Township) null;
        }
        tilesToRemove.Clear();
        foreach (Path path in pathsToRemove)
        {
          path.Dispose();
          this.worldBuilder.highwayPaths.Remove(path);
        }
        pathsToRemove.Clear();
        t = (Township) null;
      }
    }
    Log.Out($"HighwayPlanner cleanupHighwayConnections in {(ValueType) (float) ((double) ms.ElapsedMilliseconds * (1.0 / 1000.0))}, r={Rand.Instance.PeekSample():x}");
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public IEnumerator runTownshipDirtRoads()
  {
    MicroStopwatch ms = new MicroStopwatch(true);
    List<Township> countryTownships = this.worldBuilder.Townships.FindAll((Predicate<Township>) ([PublicizedFrom(EAccessModifier.Internal)] (_township) => !_township.Data.SpawnGateway));
    for (int n = 0; n < countryTownships.Count; ++n)
    {
      Township t = countryTownships[n];
      string msg = string.Format(Localization.Get("xuiRwgHighwaysTownship"), (object) (n + 1), (object) countryTownships.Count);
      yield return (object) this.worldBuilder.SetMessage(msg);
      MicroStopwatch ms2 = new MicroStopwatch(true);
      foreach (StreetTile streetTile in t.Streets.Values)
      {
        if (streetTile.GetNumTownshipNeighbors() == 1)
        {
          int num = -1;
          for (int idx = 0; idx < 4; ++idx)
          {
            StreetTile neighborByIndex = streetTile.GetNeighborByIndex(idx);
            if (neighborByIndex != null && neighborByIndex.Township == streetTile.Township)
            {
              num = idx;
              break;
            }
          }
          switch (num)
          {
            case 0:
              streetTile.SetRoadExit(2, true);
              goto label_18;
            case 1:
              streetTile.SetRoadExit(3, true);
              goto label_18;
            case 2:
              streetTile.SetRoadExit(0, true);
              goto label_18;
            case 3:
              streetTile.SetRoadExit(1, true);
              goto label_18;
            default:
              goto label_18;
          }
        }
      }
label_18:
      ms2.ResetAndRestart();
      int count = 0;
      foreach (Vector2i exit in t.GetUnusedTownExits())
      {
        Vector2i closestPoint = Vector2i.zero;
        float closestDist = float.MaxValue;
        foreach (Path highwayPath in this.worldBuilder.highwayPaths)
        {
          if (!highwayPath.isCountryRoad)
          {
            foreach (Vector2 finalPathPoint in highwayPath.FinalPathPoints)
            {
              Vector2i vector2i;
              vector2i.x = Utils.Fastfloor(finalPathPoint.x);
              vector2i.y = Utils.Fastfloor(finalPathPoint.y);
              float num = Vector2i.DistanceSqr(exit, vector2i);
              if ((double) num < (double) closestDist)
              {
                if (this.worldBuilder.PathingUtils.GetPathCost(exit, vector2i, true) > 0)
                {
                  closestDist = num;
                  closestPoint = vector2i;
                }
                ++count;
                if (this.worldBuilder.IsMessageElapsed())
                  yield return (object) this.worldBuilder.SetMessage($"{msg} {string.Format(Localization.Get("xuiRwgHighwaysTownExits"), (object) count)}");
              }
            }
          }
        }
        if (this.worldBuilder.IsMessageElapsed())
          yield return (object) this.worldBuilder.SetMessage(msg);
        Path path = new Path(this.worldBuilder, exit, closestPoint, 2, true);
        if (path.IsValid)
        {
          foreach (StreetTile streetTile in t.Streets.Values)
          {
            for (int index = 0; index < 4; ++index)
            {
              if ((double) Vector2i.Distance(streetTile.getHighwayExitPosition(index), exit) < 10.0)
                streetTile.SetExitUsed(exit);
            }
          }
          this.worldBuilder.highwayPaths.Add(path);
        }
      }
      Log.Out($"HighwayPlanner runTownshipDirtRoads #{n} unused exits c{count} in {(ValueType) (float) ((double) ms2.ElapsedMilliseconds * (1.0 / 1000.0))}, r={Rand.Instance.PeekSample():x}");
      t = (Township) null;
      msg = (string) null;
      ms2 = (MicroStopwatch) null;
    }
    Log.Out($"HighwayPlanner runTownshipDirtRoads, countryTownships {countryTownships.Count}, in {(ValueType) (float) ((double) ms.ElapsedMilliseconds * (1.0 / 1000.0))}, r={Rand.Instance.PeekSample():x}");
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
  public IEnumerator ConnectClosest(Township _township, List<Township> highwayTownships)
  {
    for (int n = 0; n < _township.Gateways.Count; ++n)
    {
      StreetTile gateway = _township.Gateways[n];
      if (gateway.UsedExitList.Count < 2)
      {
        int num1;
        List<Township> all = highwayTownships.FindAll((Predicate<Township>) ([PublicizedFrom(EAccessModifier.Internal)] (t) => (!_township.TownshipConnectionCounts.TryGetValue(t, out num1) || num1 <= 1) && t.ID != _township.ID && t.Data.SpawnGateway));
        all.Sort((Comparison<Township>) ([PublicizedFrom(EAccessModifier.Internal)] (_t1, _t2) => Vector2i.DistanceSqr(gateway.GridPosition, _t1.GridCenter).CompareTo(Vector2i.DistanceSqr(gateway.GridPosition, _t2.GridCenter))));
        Path closePath = (Path) null;
        Township closeTownship = (Township) null;
        int tries = 0;
        foreach (Township townshipNear in all)
        {
          yield return (object) this.GetPathToTownship(gateway, townshipNear);
          Path toTownshipResult = this.getPathToTownshipResult;
          if (toTownshipResult != null)
          {
            this.getPathToTownshipResult = (Path) null;
            int num2;
            _township.TownshipConnectionCounts.TryGetValue(townshipNear, out num2);
            if (_township.Streets.Count <= 1 || townshipNear.Streets.Count <= 1)
            {
              if (num2 > 0)
                toTownshipResult.Cost *= 4;
            }
            else if (num2 > 0)
              toTownshipResult.Cost = (int) ((double) toTownshipResult.Cost * 1.6000000238418579);
            if (closePath == null || toTownshipResult.Cost < closePath.Cost)
            {
              closePath = toTownshipResult;
              closeTownship = townshipNear;
            }
            if (++tries >= 3)
              break;
          }
        }
        if (closePath != null)
        {
          int num3;
          _township.TownshipConnectionCounts.TryGetValue(closeTownship, out num3);
          _township.TownshipConnectionCounts[closeTownship] = num3 + 1;
          closeTownship.TownshipConnectionCounts.TryGetValue(_township, out num3);
          closeTownship.TownshipConnectionCounts[_township] = num3 + 1;
          this.worldBuilder.highwayPaths.Add(closePath);
          this.SetTileExits(closePath);
          closePath.commitPathingMapData();
        }
        closePath = (Path) null;
        closeTownship = (Township) null;
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public IEnumerator ConnectSelf(Township _township)
  {
    _township.SortGatewaysClockwise();
    int count = _township.Gateways.Count;
    for (int n = 0; n < count; ++n)
    {
      StreetTile gateway = _township.Gateways[n];
      if (gateway.UsedExitList.Count < 4)
      {
        StreetTile gateway2 = _township.Gateways[(n + 1) % count];
        if (gateway2.UsedExitList.Count < 4)
        {
          int closeDist = int.MaxValue;
          Path closePath = (Path) null;
          foreach (Vector2i highwayExit1 in gateway.GetHighwayExits(true))
          {
            foreach (Vector2i highwayExit2 in gateway2.GetHighwayExits(true))
            {
              Path path = new Path(this.worldBuilder, highwayExit1, highwayExit2, 4, false);
              if (path.IsValid)
              {
                int cost = path.Cost;
                if (cost < closeDist)
                {
                  closeDist = cost;
                  closePath = path;
                }
              }
              path.Dispose();
            }
            if (this.worldBuilder.IsMessageElapsed())
              yield return (object) this.worldBuilder.SetMessage(string.Format(Localization.Get("xuiRwgHighwaysTownExitsSelf"), Application.isEditor ? (object) gateway.Township.GetTypeName() : (object) string.Empty));
          }
          if (closePath != null)
          {
            this.worldBuilder.highwayPaths.Add(closePath);
            this.SetTileExits(closePath);
            closePath.commitPathingMapData();
          }
          gateway = (StreetTile) null;
          gateway2 = (StreetTile) null;
          closePath = (Path) null;
        }
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void SetTileExits(Path path)
  {
    this.SetTileExit(path, path.StartPosition);
    this.SetTileExit(path, path.EndPosition);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void SetTileExit(Path currentPath, Vector2i exit)
  {
    StreetTile parent = this.worldBuilder.GetStreetTileWorld(exit);
    if (parent != null)
    {
      if (parent.District != null && parent.District.name == "gateway")
      {
        this.ExitConnections.Add(new HighwayPlanner.ExitConnection(parent, exit, currentPath));
        return;
      }
      foreach (StreetTile neighbor in parent.GetNeighbors())
      {
        if (neighbor != null && neighbor.District != null && neighbor.District.name == "gateway")
        {
          this.ExitConnections.Add(new HighwayPlanner.ExitConnection(neighbor, exit, currentPath));
          return;
        }
      }
      parent = (StreetTile) null;
    }
    if (parent != null)
      return;
    Township township1 = (Township) null;
    foreach (Township township2 in this.worldBuilder.Townships)
    {
      if (township2.Area.Contains(exit.AsVector2()))
      {
        township1 = township2;
        break;
      }
    }
    if (township1 == null)
      return;
    foreach (StreetTile gateway in township1.Gateways)
    {
      for (int index = 0; index < 4; ++index)
      {
        if (gateway.getHighwayExitPosition(index) == exit || (double) Vector2i.DistanceSqr(gateway.getHighwayExitPosition(index), exit) < 100.0)
        {
          this.ExitConnections.Add(new HighwayPlanner.ExitConnection(gateway, exit, currentPath));
          return;
        }
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static int GetDist(Township thisTownship, Township otherTownship, ref int closestDist)
  {
    foreach (Vector2i unusedTownExit1 in thisTownship.GetUnusedTownExits())
    {
      foreach (Vector2i unusedTownExit2 in otherTownship.GetUnusedTownExits())
      {
        int num = Vector2i.DistanceSqrInt(unusedTownExit1, unusedTownExit2);
        if (num < closestDist)
          closestDist = num;
      }
    }
    return closestDist;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public IEnumerator GetPathToTownship(StreetTile gateway, Township otherTownship)
  {
    int closestDist = int.MaxValue;
    Path closestPath = (Path) null;
    foreach (Vector2i highwayExit in gateway.GetHighwayExits(true))
    {
      foreach (Vector2i unusedTownExit in otherTownship.GetUnusedTownExits(3))
      {
        Path path = new Path(this.worldBuilder, highwayExit, unusedTownExit, 4, false);
        if (path.IsValid)
        {
          int cost = path.Cost;
          if (cost < closestDist)
          {
            closestDist = cost;
            closestPath = path;
          }
        }
        path.Dispose();
      }
      if (this.worldBuilder.IsMessageElapsed())
        yield return (object) this.worldBuilder.SetMessage(string.Format(Localization.Get("xuiRwgHighwaysTownExitsOther"), Application.isEditor ? (object) gateway.Township.GetTypeName() : (object) string.Empty));
    }
    this.getPathToTownshipResult = closestPath;
  }

  public enum CDirs
  {
    Invalid = -1, // 0xFFFFFFFF
    North = 0,
    East = 1,
    South = 2,
    West = 3,
  }

  public class ExitConnection
  {
    public StreetTile ParentTile;
    public Vector2i WorldPosition;
    public int ExitDir = -1;
    public Path ConnectedPath;

    public ExitConnection(StreetTile parent, Vector2i worldPos, Path connectedPath = null)
    {
      this.ParentTile = parent;
      this.WorldPosition = worldPos;
      this.ConnectedPath = connectedPath;
      for (int index = 0; index < 4; ++index)
      {
        if (parent.getHighwayExitPosition(index) == this.WorldPosition)
        {
          this.ExitDir = index;
          break;
        }
      }
      parent.SetExitUsed(this.WorldPosition);
    }

    public bool SetExitUsedManually()
    {
      return this.ParentTile.UsedExitList.Contains(this.ParentTile.getHighwayExitPosition(this.ExitDir)) && (this.ParentTile.ConnectedExits & 1 << this.ExitDir) > 0 && this.ParentTile.RoadExits[this.ExitDir] || this.ParentTile.SetExitUsed(this.WorldPosition);
    }
  }
}
