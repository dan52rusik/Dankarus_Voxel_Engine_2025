// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.PathingUtils
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using UnityEngine;

#nullable disable
namespace WorldGenerationEngineFinal;

public class PathingUtils
{
  public const int PATHING_GRID_TILE_SIZE = 10;
  public const int stepSize = 10;
  public static readonly Vector2i stepHalf = new Vector2i(5, 5);
  [PublicizedFrom(EAccessModifier.Private)]
  public const float cRoadCountryMaxStepH = 12f;
  [PublicizedFrom(EAccessModifier.Private)]
  public const float cRoadHighwayMaxStepH = 11f;
  [PublicizedFrom(EAccessModifier.Private)]
  public const float cHeightCostScale = 10f;
  [PublicizedFrom(EAccessModifier.Private)]
  public const int cNormalNeighborsCount = 8;
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly Vector2i[] normalNeighbors = new Vector2i[8]
  {
    new Vector2i(0, 1),
    new Vector2i(1, 1),
    new Vector2i(1, 0),
    new Vector2i(1, -1),
    new Vector2i(0, -1),
    new Vector2i(-1, -1),
    new Vector2i(-1, 0),
    new Vector2i(-1, 1)
  };
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly Vector2i[] normalNeighbors4way = new Vector2i[4]
  {
    new Vector2i(0, 1),
    new Vector2i(1, 0),
    new Vector2i(0, -1),
    new Vector2i(-1, 0)
  };
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly WorldBuilder worldBuilder;
  [PublicizedFrom(EAccessModifier.Private)]
  public sbyte[] pathingGrid;
  [PublicizedFrom(EAccessModifier.Private)]
  public int pathingGridSize;
  [PublicizedFrom(EAccessModifier.Private)]
  public List<Vector2i> pathTemp = new List<Vector2i>(200);
  [PublicizedFrom(EAccessModifier.Private)]
  public bool[] closedListTemp;
  [PublicizedFrom(EAccessModifier.Private)]
  public int closedListTempWidth;
  [PublicizedFrom(EAccessModifier.Private)]
  public int closedListMinY;
  [PublicizedFrom(EAccessModifier.Private)]
  public int closedListMaxY;
  [PublicizedFrom(EAccessModifier.Private)]
  public PathNodePool nodePool = new PathNodePool(100000);
  [PublicizedFrom(EAccessModifier.Private)]
  public PathingUtils.MinHeapBinned minHeapBinnedTemp;
  [PublicizedFrom(EAccessModifier.Private)]
  public Vector2i wPos;

  public PathingUtils(WorldBuilder _worldBuilder) => this.worldBuilder = _worldBuilder;

  public int GetPathCost(Vector2i start, Vector2i end, bool isCountryRoad = false)
  {
    PathNode pathNode = this.FindDetailedPath(start / 10, end / 10, isCountryRoad, false);
    int pathCost = 0;
    for (; pathNode != null; pathNode = pathNode.next)
      ++pathCost;
    this.nodePool.ReturnAll();
    return pathCost;
  }

  public List<Vector2i> GetPath(
    Vector2i _start,
    Vector2i _end,
    bool _isCountryRoad,
    bool _isRiver = false)
  {
    PathNode pathNode = this.FindDetailedPath(_start / 10, _end / 10, _isCountryRoad, _isRiver);
    this.pathTemp.Clear();
    for (; pathNode != null; pathNode = pathNode.next)
      this.pathTemp.Add(pathNode.position * 10 + PathingUtils.stepHalf);
    this.nodePool.ReturnAll();
    return this.pathTemp;
  }

  public Vector2i GetPathPoint(
    Vector2i _start,
    List<Vector2> _endPath,
    bool _isCountryRoad,
    bool _isRiver,
    out int _cost)
  {
    this.ConvertPathToTemp(_endPath);
    Vector2i pathPoint = Vector2i.min;
    int num = 0;
    PathNode pathNode = this.FindDetailedPath(_start / 10, this.pathTemp, _isCountryRoad, _isRiver);
    if (pathNode != null)
    {
      pathPoint = pathNode.position * 10 + PathingUtils.stepHalf;
      Vector2 _destPoint;
      if ((double) PathingUtils.FindClosestPathPoint(_endPath, pathPoint.AsVector2(), out _destPoint) < 400.0)
        pathPoint = new Vector2i(_destPoint);
      for (; pathNode != null; pathNode = pathNode.next)
        ++num;
    }
    this.nodePool.ReturnAll();
    _cost = num;
    return pathPoint;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void ConvertPathToTemp(List<Vector2> _path)
  {
    this.pathTemp.Clear();
    for (int index = 0; index < _path.Count; ++index)
    {
      Vector2i vector2i;
      vector2i.x = ((int) _path[index].x + PathingUtils.stepHalf.x) / 10;
      vector2i.y = ((int) _path[index].y + PathingUtils.stepHalf.y) / 10;
      this.pathTemp.Add(vector2i);
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public PathNode FindDetailedPath(
    Vector2i startPos,
    Vector2i endPos,
    bool _isCountryRoad,
    bool _isRiver)
  {
    if (!this.InBounds(startPos) || !this.InBounds(endPos))
      return (PathNode) null;
    int num1 = this.worldBuilder.WorldSize / 10 + 1;
    if (this.closedListTemp != null)
    {
      int index = this.closedListMinY * num1;
      int num2 = this.closedListMaxY * num1;
      Array.Clear((Array) this.closedListTemp, index, num2 - index + num1);
    }
    if (this.closedListTemp == null || this.closedListTempWidth != num1)
    {
      this.closedListTempWidth = num1;
      this.closedListTemp = new bool[num1 * num1];
    }
    bool[] closedListTemp = this.closedListTemp;
    if (this.minHeapBinnedTemp == null)
      this.minHeapBinnedTemp = new PathingUtils.MinHeapBinned();
    PathingUtils.MinHeapBinned minHeapBinnedTemp = this.minHeapBinnedTemp;
    minHeapBinnedTemp.Reset();
    PathNode pathNode1 = this.nodePool.Alloc();
    pathNode1.Set(startPos, 0.0f, (PathNode) null);
    minHeapBinnedTemp.Add(pathNode1);
    closedListTemp[startPos.x + startPos.y * num1] = true;
    this.closedListMinY = startPos.y;
    this.closedListMaxY = startPos.y;
    Vector2i vector2i1 = new Vector2i(Utils.FastMin(startPos.x, endPos.x), Utils.FastMin(startPos.y, endPos.y));
    Vector2i vector2i2 = new Vector2i(Utils.FastMax(startPos.x, endPos.x), Utils.FastMax(startPos.y, endPos.y));
    int num3 = 200;
    int num4 = Utils.FastMax(0, vector2i1.x - num3);
    int num5 = Utils.FastMax(0, vector2i1.y - num3);
    int num6 = Utils.FastMin(vector2i2.x + num3, num1 - 1);
    int num7 = Utils.FastMin(vector2i2.y + num3, num1 - 1);
    float num8 = _isCountryRoad ? 12f : 11f;
    int num9 = 20000;
    PathNode first;
    while ((first = minHeapBinnedTemp.ExtractFirst()) != null && --num9 >= 0)
    {
      Vector2i position = first.position;
      if (position == endPos)
        return first;
      for (int index1 = 0; index1 < 8; ++index1)
      {
        Vector2i normalNeighbor = this.normalNeighbors[index1];
        Vector2i vector2i3 = first.position + normalNeighbor;
        if (vector2i3.x >= num4 && vector2i3.y >= num5 && vector2i3.x < num6 && vector2i3.y < num7)
        {
          int index2 = vector2i3.x + vector2i3.y * num1;
          if (!closedListTemp[index2])
          {
            if (vector2i3 != endPos)
            {
              bool flag = this.IsPathBlocked(vector2i3.x, vector2i3.y);
              if (!flag)
                flag = this.IsBlocked(vector2i3.x, vector2i3.y, _isRiver);
              if (flag)
              {
                closedListTemp[index2] = true;
                this.closedListMinY = Utils.FastMin(this.closedListMinY, vector2i3.y);
                this.closedListMaxY = Utils.FastMax(this.closedListMaxY, vector2i3.y);
                continue;
              }
            }
            float num10 = Utils.FastAbs(this.GetHeight(position) - this.GetHeight(vector2i3));
            if ((double) num10 <= (double) num8)
            {
              float num11 = num10 * 10f;
              float num12 = Vector2i.Distance(vector2i3, endPos) + num11;
              if (!_isCountryRoad)
              {
                StreetTile streetTileWorld = this.worldBuilder.GetStreetTileWorld(vector2i3 * 10);
                if (streetTileWorld != null && streetTileWorld.ContainsHighway)
                {
                  if (streetTileWorld.ConnectedHighways.Count <= 2)
                  {
                    if ((vector2i3.x != endPos.x || vector2i3.y != endPos.y) && (vector2i3.x != startPos.x || vector2i3.y != startPos.y))
                    {
                      PathTile pathTile1 = this.worldBuilder.PathingGrid[vector2i3.x, vector2i3.y];
                      bool flag1 = pathTile1 != null && pathTile1.TileState == PathTile.PathTileStates.Highway;
                      if (normalNeighbor.x != 0 && normalNeighbor.y != 0)
                      {
                        for (int index3 = 0; index3 < 2; ++index3)
                        {
                          Vector2i vector2i4;
                          if (index3 != 0)
                          {
                            if (index3 != 1)
                              throw new IndexOutOfRangeException("FindDetailedPath direction loop iterating past defined Vectors");
                            vector2i4 = new Vector2i(0, -normalNeighbor.y);
                          }
                          else
                            vector2i4 = new Vector2i(-normalNeighbor.x, 0);
                          Vector2i vector2i5 = vector2i4;
                          int index4 = vector2i3.x + vector2i5.x;
                          int index5 = vector2i3.y + vector2i5.y;
                          bool flag2 = this.IsPathBlocked(index4, index5);
                          if (!flag2)
                            flag2 = this.IsBlocked(index4, index5);
                          if (flag2)
                          {
                            flag1 = true;
                          }
                          else
                          {
                            PathTile pathTile2 = this.worldBuilder.PathingGrid[index4, index5];
                            if (pathTile2 != null && pathTile2.TileState == PathTile.PathTileStates.Highway)
                              flag1 = true;
                          }
                        }
                      }
                      if (flag1)
                        continue;
                    }
                    num12 *= 2f;
                  }
                  else
                    continue;
                }
              }
              if (normalNeighbor.x != 0 && normalNeighbor.y != 0)
                num12 *= 1.2f;
              if (this.pathingGrid != null)
              {
                int num13 = (int) this.pathingGrid[vector2i3.x + vector2i3.y * this.pathingGridSize];
                if (num13 > 0)
                  num12 *= (float) num13;
              }
              closedListTemp[index2] = true;
              this.closedListMinY = Utils.FastMin(this.closedListMinY, vector2i3.y);
              this.closedListMaxY = Utils.FastMax(this.closedListMaxY, vector2i3.y);
              PathNode pathNode2 = this.nodePool.Alloc();
              pathNode2.Set(vector2i3, first.pathCost + num12, first);
              minHeapBinnedTemp.Add(pathNode2);
            }
          }
        }
      }
    }
    return (PathNode) null;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public PathNode FindDetailedPath(
    Vector2i startPos,
    List<Vector2i> _endPath,
    bool _isCountryRoad,
    bool _isRiver)
  {
    int num1 = this.worldBuilder.WorldSize / 10 + 1;
    if (this.closedListTemp != null)
    {
      int index = this.closedListMinY * num1;
      int num2 = this.closedListMaxY * num1;
      Array.Clear((Array) this.closedListTemp, index, num2 - index + num1);
    }
    if (this.closedListTemp == null || this.closedListTempWidth != num1)
    {
      this.closedListTempWidth = num1;
      this.closedListTemp = new bool[num1 * num1];
    }
    bool[] closedListTemp = this.closedListTemp;
    if (this.minHeapBinnedTemp == null)
      this.minHeapBinnedTemp = new PathingUtils.MinHeapBinned();
    PathingUtils.MinHeapBinned minHeapBinnedTemp = this.minHeapBinnedTemp;
    minHeapBinnedTemp.Reset();
    PathNode pathNode1 = this.nodePool.Alloc();
    pathNode1.Set(startPos, 0.0f, (PathNode) null);
    minHeapBinnedTemp.Add(pathNode1);
    closedListTemp[startPos.x + startPos.y * num1] = true;
    this.closedListMinY = startPos.y;
    this.closedListMaxY = startPos.y;
    Vector2i _min;
    Vector2i _max;
    PathingUtils.CalcPathBounds(_endPath, out _min, out _max);
    int num3 = 200;
    int num4 = Utils.FastMax(0, _min.x - num3);
    int num5 = Utils.FastMax(0, _min.y - num3);
    int num6 = Utils.FastMin(_max.x + num3, num1 - 1);
    int num7 = Utils.FastMin(_max.y + num3, num1 - 1);
    float num8 = _isCountryRoad ? 12f : 11f;
    int num9 = 20000;
    PathNode first;
    while ((first = minHeapBinnedTemp.ExtractFirst()) != null && --num9 >= 0)
    {
      Vector2i position = first.position;
      if (PathingUtils.IsPointOnPath(_endPath, position))
        return first;
      for (int index1 = 0; index1 < 8; ++index1)
      {
        Vector2i normalNeighbor = this.normalNeighbors[index1];
        Vector2i vector2i1 = first.position + normalNeighbor;
        if (vector2i1.x >= num4 && vector2i1.y >= num5 && vector2i1.x < num6 && vector2i1.y < num7)
        {
          int index2 = vector2i1.x + vector2i1.y * num1;
          if (!closedListTemp[index2])
          {
            Vector2i _destPoint;
            double closestPathPoint = (double) PathingUtils.FindClosestPathPoint(_endPath, vector2i1, out _destPoint);
            if (vector2i1 != _destPoint)
            {
              bool flag = this.IsPathBlocked(vector2i1.x, vector2i1.y);
              if (!flag)
                flag = this.IsBlocked(vector2i1.x, vector2i1.y, _isRiver);
              if (flag)
              {
                closedListTemp[index2] = true;
                this.closedListMinY = Utils.FastMin(this.closedListMinY, vector2i1.y);
                this.closedListMaxY = Utils.FastMax(this.closedListMaxY, vector2i1.y);
                continue;
              }
            }
            float num10 = Utils.FastAbs(this.GetHeight(position) - this.GetHeight(vector2i1));
            if ((double) num10 <= (double) num8)
            {
              float num11 = num10 * 10f;
              float num12 = Vector2i.Distance(vector2i1, _destPoint) + num11;
              if (!_isCountryRoad)
              {
                StreetTile streetTileWorld = this.worldBuilder.GetStreetTileWorld(vector2i1 * 10);
                if (streetTileWorld != null && streetTileWorld.ContainsHighway)
                {
                  if (streetTileWorld.ConnectedHighways.Count <= 2)
                  {
                    if ((vector2i1.x != _destPoint.x || vector2i1.y != _destPoint.y) && (vector2i1.x != startPos.x || vector2i1.y != startPos.y))
                    {
                      PathTile pathTile1 = this.worldBuilder.PathingGrid[vector2i1.x, vector2i1.y];
                      bool flag1 = pathTile1 != null && pathTile1.TileState == PathTile.PathTileStates.Highway;
                      if (normalNeighbor.x != 0 && normalNeighbor.y != 0)
                      {
                        for (int index3 = 0; index3 < 2; ++index3)
                        {
                          Vector2i vector2i2;
                          if (index3 != 0)
                          {
                            if (index3 != 1)
                              throw new IndexOutOfRangeException("FindDetailedPath direction loop iterating past defined Vectors");
                            vector2i2 = new Vector2i(0, -normalNeighbor.y);
                          }
                          else
                            vector2i2 = new Vector2i(-normalNeighbor.x, 0);
                          Vector2i vector2i3 = vector2i2;
                          int index4 = vector2i1.x + vector2i3.x;
                          int index5 = vector2i1.y + vector2i3.y;
                          bool flag2 = this.IsPathBlocked(index4, index5);
                          if (!flag2)
                            flag2 = this.IsBlocked(index4, index5);
                          if (flag2)
                          {
                            flag1 = true;
                          }
                          else
                          {
                            PathTile pathTile2 = this.worldBuilder.PathingGrid[index4, index5];
                            if (pathTile2 != null && pathTile2.TileState == PathTile.PathTileStates.Highway)
                              flag1 = true;
                          }
                        }
                      }
                      if (flag1)
                        continue;
                    }
                    num12 *= 2f;
                  }
                  else
                    continue;
                }
              }
              if (normalNeighbor.x != 0 && normalNeighbor.y != 0)
                num12 *= 1.2f;
              if (this.pathingGrid != null)
              {
                int num13 = (int) this.pathingGrid[vector2i1.x + vector2i1.y * this.pathingGridSize];
                if (num13 > 0)
                  num12 *= (float) num13;
              }
              closedListTemp[index2] = true;
              this.closedListMinY = Utils.FastMin(this.closedListMinY, vector2i1.y);
              this.closedListMaxY = Utils.FastMax(this.closedListMaxY, vector2i1.y);
              PathNode pathNode2 = this.nodePool.Alloc();
              pathNode2.Set(vector2i1, first.pathCost + num12, first);
              minHeapBinnedTemp.Add(pathNode2);
            }
          }
        }
      }
    }
    return (PathNode) null;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static void CalcPathBounds(List<Vector2i> _path, out Vector2i _min, out Vector2i _max)
  {
    _min = Vector2i.max;
    _max = Vector2i.min;
    foreach (Vector2i vector2i in _path)
    {
      _min.x = Utils.FastMin(_min.x, vector2i.x);
      _min.y = Utils.FastMin(_min.y, vector2i.y);
      _max.x = Utils.FastMax(_max.x, vector2i.x);
      _max.y = Utils.FastMax(_max.y, vector2i.y);
    }
  }

  public static float FindClosestPathPoint(
    List<Vector2> _path,
    Vector2 _startPos,
    out Vector2 _destPoint,
    int _step = 1)
  {
    _destPoint = Vector2.zero;
    float closestPathPoint = float.MaxValue;
    int count = _path.Count;
    for (int index = 0; index < count; index += _step)
    {
      Vector2 vector2 = _path[index];
      float sqrMagnitude = (_startPos - vector2).sqrMagnitude;
      if ((double) sqrMagnitude < (double) closestPathPoint)
      {
        closestPathPoint = sqrMagnitude;
        _destPoint = vector2;
      }
    }
    return closestPathPoint;
  }

  public static float FindClosestPathPoint(
    List<Vector2i> _path,
    Vector2i _startPos,
    out Vector2i _destPoint)
  {
    _destPoint = Vector2i.zero;
    float closestPathPoint = float.MaxValue;
    foreach (Vector2i b in _path)
    {
      float num = Vector2i.DistanceSqr(_startPos, b);
      if ((double) num < (double) closestPathPoint)
      {
        closestPathPoint = num;
        _destPoint = b;
      }
    }
    return closestPathPoint;
  }

  public static Vector2 FindPathPoint(List<Vector2> _path, float _percent)
  {
    int index = (int) ((double) (_path.Count - 1) * (double) _percent);
    return _path[index];
  }

  public static bool IsPointOnPath(List<Vector2i> _path, Vector2i _point)
  {
    foreach (Vector2i vector2i in _path)
    {
      if (vector2i.x == _point.x && vector2i.y == _point.y)
        return true;
    }
    return false;
  }

  public bool IsBlocked(int pathX, int pathY, bool isRiver = false)
  {
    Vector2i worldCenter = this.pathPositionToWorldCenter(pathX, pathY);
    if (!this.InWorldBounds(worldCenter.x, worldCenter.y))
      return true;
    StreetTile streetTileWorld = this.worldBuilder.GetStreetTileWorld(worldCenter.x, worldCenter.y);
    return this.InCityLimits(streetTileWorld) || this.IsRadiation(streetTileWorld) || !isRiver && this.IsWater(pathX, pathY);
  }

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public bool InBounds(Vector2i pos) => this.InBounds(pos.x, pos.y);

  public bool InBounds(int pathX, int pathY)
  {
    Vector2i worldCenter = this.pathPositionToWorldCenter(pathX, pathY);
    return (long) (uint) worldCenter.x < (long) this.worldBuilder.WorldSize && (long) (uint) worldCenter.y < (long) this.worldBuilder.WorldSize;
  }

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public bool InWorldBounds(int x, int y)
  {
    return (long) (uint) x < (long) this.worldBuilder.WorldSize && (long) (uint) y < (long) this.worldBuilder.WorldSize;
  }

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public bool IsRadiation(StreetTile st)
  {
    return (st == null || st.OverlapsRadiation) && this.worldBuilder.GetRad(this.wPos.x, this.wPos.y) > (byte) 0;
  }

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public bool InCityLimits(StreetTile st)
  {
    return st != null && st.Township != null && !st.Township.IsWilderness();
  }

  public bool IsWater(Vector2i pos) => this.IsWater(pos.x, pos.y);

  public bool IsWater(int pathX, int pathY)
  {
    Vector2i worldMin = this.pathPositionToWorldMin(pathX, pathY);
    StreetTile streetTileWorld = this.worldBuilder.GetStreetTileWorld(worldMin);
    if (streetTileWorld == null || streetTileWorld.OverlapsWater)
    {
      for (int y = worldMin.y; y < worldMin.y + 10; ++y)
      {
        for (int x = worldMin.x; x < worldMin.x + 10; ++x)
        {
          if (this.worldBuilder.GetWater(x, y) > (byte) 0)
            return true;
        }
      }
    }
    return false;
  }

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public float GetHeight(Vector2i pos) => this.GetHeight(pos.x, pos.y);

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public float GetHeight(int pathX, int pathY)
  {
    return this.worldBuilder.GetHeight(this.pathPositionToWorldCenter(pathX, pathY));
  }

  public BiomeType GetBiome(Vector2i pos) => this.GetBiome(pos.x, pos.y);

  public BiomeType GetBiome(int pathX, int pathY)
  {
    return this.worldBuilder.GetBiome(this.pathPositionToWorldCenter(pathX, pathY));
  }

  [PublicizedFrom(EAccessModifier.Private)]
  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public Vector2i pathPositionToWorldCenter(int pathX, int pathY)
  {
    this.wPos.x = pathX * 10 + 5;
    this.wPos.y = pathY * 10 + 5;
    return this.wPos;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public Vector2i pathPositionToWorldMin(int pathX, int pathY)
  {
    this.wPos.x = pathX * 10;
    this.wPos.y = pathY * 10;
    return this.wPos;
  }

  public void AddMoveLimitArea(Rect r)
  {
    int xMin = (int) r.xMin;
    int yMin = (int) r.yMin;
    int num1 = xMin / 10;
    int num2 = yMin / 10;
    for (int index1 = 0; index1 < 15; ++index1)
    {
      for (int index2 = 0; index2 < 15; ++index2)
      {
        if (index2 != 7 && index1 != 7)
          this.SetPathBlocked(num1 + index2, num2 + index1, true);
      }
    }
  }

  public void RemoveFullyBlockedArea(Rect r)
  {
    int xMin = (int) r.xMin;
    int yMin = (int) r.yMin;
    int num1 = xMin / 10;
    int num2 = yMin / 10;
    for (int index1 = 0; index1 < 15; ++index1)
    {
      for (int index2 = 0; index2 < 15; ++index2)
        this.SetPathBlocked(num1 + index2, num2 + index1, false);
    }
  }

  public void AddFullyBlockedArea(Rect r)
  {
    int num1 = (int) ((double) r.xMin + 0.5);
    int num2 = (int) ((double) r.yMin + 0.5);
    int num3 = num1 / 10;
    int num4 = num2 / 10;
    for (int index1 = 0; index1 < 15; ++index1)
    {
      for (int index2 = 0; index2 < 15; ++index2)
        this.SetPathBlocked(num3 + index2, num4 + index1, true);
    }
  }

  public void SetPathBlocked(Vector2i pos, bool isBlocked)
  {
    this.SetPathBlocked(pos.x, pos.y, isBlocked);
  }

  public void SetPathBlocked(int x, int y, bool isBlocked)
  {
    this.SetPathBlocked(x, y, isBlocked ? sbyte.MinValue : (sbyte) 0);
  }

  public void SetPathBlocked(int x, int y, sbyte costMult)
  {
    if (this.pathingGrid == null)
      this.SetupPathingGrid();
    if ((long) (uint) x >= (long) this.pathingGridSize || (long) (uint) y >= (long) this.pathingGridSize)
      return;
    this.pathingGrid[x + y * this.pathingGridSize] = costMult;
  }

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public bool IsPathBlocked(int x, int y)
  {
    return (long) (uint) x >= (long) this.pathingGridSize || (long) (uint) y >= (long) this.pathingGridSize || this.pathingGrid[x + y * this.pathingGridSize] == sbyte.MinValue;
  }

  public bool IsPointOnHighwayWorld(int x, int y)
  {
    return this.worldBuilder.PathingGrid[x / 10, y / 10] != null && this.worldBuilder.PathingGrid[x, y].TileState == PathTile.PathTileStates.Highway;
  }

  public bool IsPointOnCountryRoadWorld(int x, int y)
  {
    return this.worldBuilder.PathingGrid[x / 10, y / 10] != null && this.worldBuilder.PathingGrid[x, y].TileState == PathTile.PathTileStates.Country;
  }

  public void SetupPathingGrid()
  {
    this.pathingGridSize = this.worldBuilder.WorldSize / 10;
    this.pathingGrid = new sbyte[this.pathingGridSize * this.pathingGridSize];
  }

  public void Cleanup()
  {
    this.closedListTemp = (bool[]) null;
    this.pathingGrid = (sbyte[]) null;
    this.pathingGridSize = 0;
    this.minHeapBinnedTemp = (PathingUtils.MinHeapBinned) null;
    this.nodePool.Cleanup();
  }

  [PublicizedFrom(EAccessModifier.Private)]
  static PathingUtils()
  {
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public class MinHeap
  {
    [PublicizedFrom(EAccessModifier.Private)]
    public PathNode listHead;

    public void Add(PathNode item)
    {
      if (this.listHead == null)
        this.listHead = item;
      else if (this.listHead.next == null && (double) item.pathCost <= (double) this.listHead.pathCost)
      {
        item.nextListElem = this.listHead;
        this.listHead = item;
      }
      else
      {
        PathNode pathNode = this.listHead;
        PathNode nextListElem;
        for (nextListElem = pathNode.nextListElem; nextListElem != null && (double) nextListElem.pathCost < (double) item.pathCost; nextListElem = pathNode.nextListElem)
          pathNode = nextListElem;
        item.nextListElem = nextListElem;
        pathNode.nextListElem = item;
      }
    }

    public PathNode ExtractFirst()
    {
      PathNode listHead = this.listHead;
      if (listHead == null)
        return listHead;
      this.listHead = this.listHead.nextListElem;
      return listHead;
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public class MinHeapBinned
  {
    [PublicizedFrom(EAccessModifier.Private)]
    public const int cBins = 32768 /*0x8000*/;
    [PublicizedFrom(EAccessModifier.Private)]
    public const float cScale = 0.07f;
    [PublicizedFrom(EAccessModifier.Private)]
    public PathNode[] nodeBins;
    [PublicizedFrom(EAccessModifier.Private)]
    public int lowBin = 32768 /*0x8000*/;
    [PublicizedFrom(EAccessModifier.Private)]
    public int highBin;

    public MinHeapBinned() => this.nodeBins = new PathNode[32768 /*0x8000*/];

    public void Reset()
    {
      if (this.lowBin <= this.highBin)
        Array.Clear((Array) this.nodeBins, this.lowBin, this.highBin - this.lowBin + 1);
      this.lowBin = 32768 /*0x8000*/;
      this.highBin = 0;
    }

    public PathNode ExtractFirst()
    {
      if (this.lowBin > this.highBin)
        return (PathNode) null;
      PathNode nodeBin = this.nodeBins[this.lowBin];
      this.nodeBins[this.lowBin] = nodeBin.nextListElem;
      if (nodeBin.nextListElem == null)
      {
        do
          ;
        while (++this.lowBin <= this.highBin && this.nodeBins[this.lowBin] == null);
        if (this.lowBin > this.highBin)
        {
          this.lowBin = 32768 /*0x8000*/;
          this.highBin = 0;
        }
      }
      return nodeBin;
    }

    public void Add(PathNode item)
    {
      int index = (int) ((double) item.pathCost * 0.070000000298023224);
      if (index >= 32768 /*0x8000*/)
        index = (int) short.MaxValue;
      if (index < this.lowBin)
        this.lowBin = index;
      if (index > this.highBin)
        this.highBin = index;
      PathNode nodeBin = this.nodeBins[index];
      if (nodeBin == null)
        this.nodeBins[index] = item;
      else if (nodeBin.next == null && (double) item.pathCost <= (double) nodeBin.pathCost)
      {
        item.nextListElem = nodeBin;
        this.nodeBins[index] = item;
      }
      else
      {
        PathNode pathNode = nodeBin;
        PathNode nextListElem;
        for (nextListElem = pathNode.nextListElem; nextListElem != null && (double) nextListElem.pathCost < (double) item.pathCost; nextListElem = pathNode.nextListElem)
          pathNode = nextListElem;
        item.nextListElem = nextListElem;
        pathNode.nextListElem = item;
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public enum PathNodeType
  {
    Free = 0,
    Road = 1,
    Prefab = 2,
    CityLimits = 4,
    Blocked = 8,
  }
}
