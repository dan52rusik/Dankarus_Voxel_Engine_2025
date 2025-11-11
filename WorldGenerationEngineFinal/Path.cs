// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.Path
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System.Collections.Generic;
using System.Runtime.CompilerServices;
using UnityEngine;

#nullable disable
namespace WorldGenerationEngineFinal;

public class Path
{
  [PublicizedFrom(EAccessModifier.Private)]
  public const float cSingleLaneRadius = 4.5f;
  [PublicizedFrom(EAccessModifier.Private)]
  public const int cShoulderWidth = 1;
  [PublicizedFrom(EAccessModifier.Private)]
  public const float cBlendDistCountry = 6f;
  [PublicizedFrom(EAccessModifier.Private)]
  public const float cBlendDistHighway = 10f;
  [PublicizedFrom(EAccessModifier.Private)]
  public const float cHeightSmoothAverageBias = 8f;
  [PublicizedFrom(EAccessModifier.Private)]
  public const float cHeightSmoothDecreasePer = 0.3f;
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly WorldBuilder worldBuilder;
  public bool IsPrefabPath;
  [PublicizedFrom(EAccessModifier.Private)]
  public int lanes;
  public float radius = 8f;
  public bool isCountryRoad;
  public bool isRiver;
  public bool connectsToHighway;
  public readonly Vector2i StartPosition;
  public readonly Vector2i EndPosition;
  public bool IsValid;
  public int StartPointID = -1;
  public int EndPointID = -1;
  public int Cost;
  public List<Vector2> FinalPathPoints = new List<Vector2>();
  public List<Vector3> pathPoints3d = new List<Vector3>();
  [PublicizedFrom(EAccessModifier.Private)]
  public bool validityTestOnly;
  [PublicizedFrom(EAccessModifier.Private)]
  public const int FreeId = 0;
  [PublicizedFrom(EAccessModifier.Private)]
  public const int CountryId = 1;
  [PublicizedFrom(EAccessModifier.Private)]
  public const int HighwayId = 2;
  [PublicizedFrom(EAccessModifier.Private)]
  public const int HighwayDirtId = 3;
  [PublicizedFrom(EAccessModifier.Private)]
  public const int WaterId = 4;
  [PublicizedFrom(EAccessModifier.Private)]
  public const int HighwayBlendIdMask = 128 /*0x80*/;

  public Path(
    WorldBuilder _worldBuilder,
    Vector2i _startPosition,
    Vector2i _endPosition,
    int _lanes,
    bool _isCountryRoad,
    bool _validityTest = false)
  {
    this.worldBuilder = _worldBuilder;
    this.StartPosition = _startPosition;
    this.EndPosition = _endPosition;
    this.lanes = _lanes;
    this.radius = (float) ((double) this.lanes * 0.5 * 4.5);
    this.isCountryRoad = _isCountryRoad;
    this.validityTestOnly = _validityTest;
    this.CreatePath();
  }

  public Path(
    WorldBuilder _worldBuilder,
    Vector2i _startPosition,
    Vector2i _endPosition,
    float _radius,
    bool _isCountryRoad,
    bool _validityTest = false)
  {
    this.worldBuilder = _worldBuilder;
    this.StartPosition = _startPosition;
    this.EndPosition = _endPosition;
    this.radius = _radius;
    this.lanes = Mathf.CeilToInt((float) ((double) this.radius / 4.5 * 2.0));
    this.isCountryRoad = _isCountryRoad;
    this.validityTestOnly = _validityTest;
    this.CreatePath();
  }

  public Path(WorldBuilder _worldBuilder, bool _isCountryRoad = false, float _radius = 8f, bool _validityTest = false)
  {
    this.worldBuilder = _worldBuilder;
    this.isCountryRoad = _isCountryRoad;
    this.FinalPathPoints = new List<Vector2>();
    this.radius = _radius;
    this.validityTestOnly = _validityTest;
  }

  public void Dispose()
  {
    if (this.isCountryRoad)
      return;
    StreetTile streetTile = (StreetTile) null;
    for (int index = 0; index < this.FinalPathPoints.Count; ++index)
    {
      Vector2i pos;
      pos.x = (int) this.FinalPathPoints[index].x;
      pos.y = (int) this.FinalPathPoints[index].y;
      StreetTile streetTileWorld = this.worldBuilder.GetStreetTileWorld(pos);
      if (streetTileWorld != streetTile && streetTileWorld != null)
        streetTileWorld.ConnectedHighways.Remove(this);
      streetTile = streetTileWorld;
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void ProcessPath()
  {
    int num1 = this.isCountryRoad ? 2 : 3;
    for (int index = 0; index < num1; ++index)
      this.roundOffCorners();
    float worldSize = (float) this.worldBuilder.WorldSize;
    float f = 0.0f;
    for (int index = 0; index < this.FinalPathPoints.Count; ++index)
    {
      float x = this.FinalPathPoints[index].x;
      if ((double) x < 0.0 || (double) x >= (double) worldSize)
      {
        this.IsValid = false;
        return;
      }
      float y = this.FinalPathPoints[index].y;
      if ((double) y < 0.0 || (double) y >= (double) worldSize)
      {
        this.IsValid = false;
        return;
      }
      if (index > 0)
        f += Vector2.Distance(this.FinalPathPoints[index - 1], this.FinalPathPoints[index]);
      this.pathPoints3d.Add(new Vector3(x, this.worldBuilder.GetHeight((int) x, (int) y), y));
    }
    this.Cost = Mathf.RoundToInt(f);
    float[] heights = new float[this.pathPoints3d.Count];
    if (this.isCountryRoad)
    {
      for (int index = 0; index < 4; ++index)
        this.SmoothHeights(heights);
    }
    else
    {
      float num2 = 0.0f;
      for (int index = 0; index < this.pathPoints3d.Count; ++index)
        num2 += this.pathPoints3d[index].y;
      float num3 = num2 / (float) this.pathPoints3d.Count + 8f;
      for (int index = 0; index < this.pathPoints3d.Count; ++index)
      {
        Vector3 vector3 = this.pathPoints3d[index];
        if ((double) vector3.y > (double) num3 && this.worldBuilder.poiHeightMask[(int) vector3.x + (int) vector3.z * this.worldBuilder.WorldSize] == (byte) 0)
        {
          vector3.y = (float) ((double) num3 * 0.30000001192092896 + (double) vector3.y * 0.699999988079071);
          this.pathPoints3d[index] = vector3;
        }
      }
      for (int index = 0; index < 50; ++index)
        this.SmoothHeights(heights);
    }
    this.FinalPathPoints.Clear();
    Vector2 zero = Vector2.zero;
    for (int index = 0; index < this.pathPoints3d.Count; ++index)
    {
      zero.x = (float) (int) ((double) this.pathPoints3d[index].x + 0.5);
      zero.y = (float) (int) ((double) this.pathPoints3d[index].z + 0.5);
      this.FinalPathPoints.Add(zero);
    }
  }

  public void DrawPathToRoadIds(byte[] ids)
  {
    float radius = this.radius;
    float num1 = !this.isCountryRoad ? radius + 10f : radius + 6f;
    double num2 = this.lanes >= 2 ? (double) this.radius - 1.0 : (double) this.radius;
    float num3 = this.radius * this.radius;
    float num4 = num1 * num1;
    float num5 = (float) (num2 * num2);
    for (int index1 = 0; index1 < this.FinalPathPoints.Count - 1; ++index1)
    {
      double x1 = (double) this.FinalPathPoints[index1].x;
      float y1 = this.FinalPathPoints[index1].y;
      float x2 = this.FinalPathPoints[index1 + 1].x;
      float y2 = this.FinalPathPoints[index1 + 1].y;
      int num6 = Utils.FastMax(0, (int) ((double) Utils.FastMin((float) x1, x2) - (double) num1 - 1.5));
      int num7 = Utils.FastMin((int) ((double) Utils.FastMax((float) x1, x2) + (double) num1 + 1.5), this.worldBuilder.WorldSize - 1);
      int num8 = Utils.FastMax(0, (int) ((double) Utils.FastMin(y1, y2) - (double) num1 - 1.5));
      int num9 = Utils.FastMin((int) ((double) Utils.FastMax(y1, y2) + (double) num1 + 1.5), this.worldBuilder.WorldSize - 1);
      for (int y3 = num8; y3 < num9; ++y3)
      {
        Vector2 point;
        point.y = (float) y3;
        for (int x3 = num6; x3 < num7; ++x3)
        {
          point.x = (float) x3;
          Vector2 pointOnLine;
          float f = this.GetPointToLineDistance2(point, this.FinalPathPoints[index1], this.FinalPathPoints[index1 + 1], out pointOnLine);
          float t1;
          if ((double) f < (double) num4)
          {
            t1 = Utils.FastClamp01(Vector2.Distance(this.FinalPathPoints[index1], pointOnLine) / Vector2.Distance(this.FinalPathPoints[index1], this.FinalPathPoints[index1 + 1]));
          }
          else
          {
            f = this.distanceSqr((float) x3, (float) y3, this.FinalPathPoints[index1]);
            if ((double) f < (double) num4)
            {
              float num10 = this.distanceSqr((float) x3, (float) y3, this.FinalPathPoints[index1 + 1]);
              if ((double) f <= (double) num10)
              {
                if (index1 > 0)
                {
                  float num11 = this.distanceSqr((float) x3, (float) y3, this.FinalPathPoints[index1 - 1]);
                  if ((double) f >= (double) num11 || (double) this.GetPointToLineDistance2(point, this.FinalPathPoints[index1 - 1], this.FinalPathPoints[index1], out Vector2 _) < (double) num4)
                    continue;
                }
                t1 = -1f;
              }
              else
                continue;
            }
            else
              continue;
          }
          int index2 = x3 + y3 * this.worldBuilder.WorldSize;
          if (this.isRiver)
          {
            if ((double) f <= (double) num3)
            {
              ids[index2] = (byte) 4;
              if ((double) this.worldBuilder.GetHeight(x3, y3) < (double) this.worldBuilder.WaterHeight)
              {
                this.worldBuilder.SetWater(x3, y3, (byte) this.worldBuilder.WaterHeight);
                continue;
              }
              this.worldBuilder.SetWater(x3, y3, (byte) this.worldBuilder.WaterHeight);
            }
          }
          else
          {
            int id = (int) ids[index2];
            if (id != 2 && ((double) f <= (double) num3 || (id & 128 /*0x80*/) <= 0))
            {
              if (!this.isCountryRoad)
              {
                if ((double) f > (double) num3)
                  ids[index2] |= (byte) 128 /*0x80*/;
                else
                  ids[index2] = (double) f <= (double) num5 ? (byte) 2 : (byte) 3;
              }
              else if ((double) f <= (double) num3)
                ids[index2] = (byte) 1;
            }
            else
              continue;
          }
          float height1 = this.worldBuilder.GetHeight(x3, y3);
          float v1 = 3f;
          if (!this.isRiver)
            v1 = (float) (Utils.FastMax((int) this.worldBuilder.GetWater(x3, y3), this.worldBuilder.WaterHeight) + 1);
          float num12 = this.pathPoints3d[index1].y;
          if ((double) t1 > 0.0)
            num12 = Utils.FastLerpUnclamped(num12, this.pathPoints3d[index1 + 1].y, t1);
          float a = Utils.FastMax(v1, num12);
          float num13 = Utils.FastClamp01((float) (((double) Mathf.Sqrt(f) - (double) this.radius) / ((double) num1 - (double) this.radius)));
          float t2 = num13 * num13;
          float height2 = Utils.FastLerpUnclamped(a, height1, t2);
          this.worldBuilder.SetHeightTrusted(x3, y3, height2);
        }
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public float GetPointToLineDistance2(
    Vector2 point,
    Vector2 lineStart,
    Vector2 lineEnd,
    out Vector2 pointOnLine)
  {
    Vector2 rhs;
    rhs.x = lineEnd.x - lineStart.x;
    rhs.y = lineEnd.y - lineStart.y;
    float num1 = Mathf.Sqrt((float) ((double) rhs.x * (double) rhs.x + (double) rhs.y * (double) rhs.y));
    rhs.x /= num1;
    rhs.y /= num1;
    float num2 = Vector2.Dot(point - lineStart, rhs);
    if ((double) num2 < 0.0 || (double) num2 > (double) num1)
    {
      pointOnLine = new Vector2(100000f, 100000f);
      return float.MaxValue;
    }
    pointOnLine = lineStart + num2 * rhs;
    return this.distanceSqr(point, pointOnLine);
  }

  public bool Crosses(Path path)
  {
    foreach (Vector2 finalPathPoint1 in this.FinalPathPoints)
    {
      foreach (Vector2 finalPathPoint2 in path.FinalPathPoints)
      {
        if ((double) this.distanceSqr(finalPathPoint1, finalPathPoint2) < 100.0)
          return true;
      }
    }
    return false;
  }

  public bool IsConnectedTo(Path path)
  {
    if (this.Crosses(path))
      return true;
    foreach (Vector2 finalPathPoint in path.FinalPathPoints)
    {
      if ((double) this.distanceSqr(this.StartPosition.AsVector2(), finalPathPoint) < 100.0 || (double) this.distanceSqr(this.EndPosition.AsVector2(), finalPathPoint) < 100.0)
        return true;
    }
    return false;
  }

  public bool IsConnectedToHighway()
  {
    if (this.worldBuilder.PathingUtils.IsPointOnHighwayWorld(this.StartPosition.x, this.StartPosition.y) || this.worldBuilder.PathingUtils.IsPointOnHighwayWorld(this.EndPosition.x, this.EndPosition.y))
      return true;
    foreach (Vector2 finalPathPoint in this.FinalPathPoints)
    {
      if (this.worldBuilder.PathingUtils.IsPointOnHighwayWorld((int) finalPathPoint.x, (int) finalPathPoint.y))
        return true;
    }
    return false;
  }

  [PublicizedFrom(EAccessModifier.Protected)]
  public float distanceSqr(Vector2i a, Vector2i b)
  {
    return (float) ((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
  }

  [PublicizedFrom(EAccessModifier.Protected)]
  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public float distanceSqr(Vector2 v1, Vector2 v2)
  {
    double num1 = (double) v1.x - (double) v2.x;
    float num2 = v1.y - v2.y;
    return (float) (num1 * num1 + (double) num2 * (double) num2);
  }

  [PublicizedFrom(EAccessModifier.Protected)]
  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public float distanceSqr(float x, float y, Vector2 v2)
  {
    double num1 = (double) x - (double) v2.x;
    float num2 = y - v2.y;
    return (float) (num1 * num1 + (double) num2 * (double) num2);
  }

  [PublicizedFrom(EAccessModifier.Protected)]
  public List<Vector3> romChain(
    List<Vector3> points,
    int numberOfPointsOnCurve = 5,
    float parametricSplineVal = 0.2f)
  {
    List<Vector3> vector3List = new List<Vector3>(points.Count * numberOfPointsOnCurve + 1);
    for (int index = 0; index < points.Count - 3; ++index)
      vector3List.AddRange((IEnumerable<Vector3>) this.catmulRom(points[index], points[index + 1], points[index + 2], points[index + 3], numberOfPointsOnCurve, parametricSplineVal));
    return vector3List;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public List<Vector3> catmulRom(
    Vector3 p0,
    Vector3 p1,
    Vector3 p2,
    Vector3 p3,
    int numberOfPointsOnCurve,
    float parametricSplineVal)
  {
    List<Vector3> vector3List = new List<Vector3>(numberOfPointsOnCurve + 1);
    float t1 = this.getT(0.0f, p0, p1, parametricSplineVal);
    float t2 = this.getT(t1, p1, p2, parametricSplineVal);
    float t3 = this.getT(t2, p2, p3, parametricSplineVal);
    for (float num = t1; (double) num < (double) t2; num += (t2 - t1) / (float) numberOfPointsOnCurve)
    {
      Vector3 vector3_1 = (float) (((double) t1 - (double) num) / ((double) t1 - 0.0)) * p0 + (float) (((double) num - 0.0) / ((double) t1 - 0.0)) * p1;
      Vector3 vector3_2 = (float) (((double) t2 - (double) num) / ((double) t2 - (double) t1)) * p1 + (float) (((double) num - (double) t1) / ((double) t2 - (double) t1)) * p2;
      Vector3 vector3_3 = (float) (((double) t3 - (double) num) / ((double) t3 - (double) t2)) * p2 + (float) (((double) num - (double) t2) / ((double) t3 - (double) t2)) * p3;
      Vector3 vector3_4 = (float) (((double) t2 - (double) num) / ((double) t2 - 0.0)) * vector3_1 + (float) (((double) num - 0.0) / ((double) t2 - 0.0)) * vector3_2;
      Vector3 vector3_5 = (float) (((double) t3 - (double) num) / ((double) t3 - (double) t1)) * vector3_2 + (float) (((double) num - (double) t1) / ((double) t3 - (double) t1)) * vector3_3;
      Vector3 vector3_6 = (float) (((double) t2 - (double) num) / ((double) t2 - (double) t1)) * vector3_4 + (float) (((double) num - (double) t1) / ((double) t2 - (double) t1)) * vector3_5;
      vector3List.Add(vector3_6);
    }
    return vector3List;
  }

  [PublicizedFrom(EAccessModifier.Protected)]
  public float getT(float t, Vector3 p0, Vector3 p1, float alpha)
  {
    return p0 == p1 ? t : Mathf.Pow((p1 - p0).sqrMagnitude, 0.5f * alpha) + t;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void CreatePath()
  {
    if (this.StartPosition.x < 8 || this.StartPosition.y < 8)
      Log.Error("Start position oob");
    else if (this.EndPosition.x < 8 || this.EndPosition.y < 8)
    {
      Log.Error("End position oob");
    }
    else
    {
      List<Vector2i> path = this.worldBuilder.PathingUtils.GetPath(this.StartPosition, this.EndPosition, this.isCountryRoad, this.isRiver);
      if (path == null || path.Count <= 2)
        return;
      this.IsValid = true;
      if (this.validityTestOnly)
        return;
      this.FinalPathPoints = new List<Vector2>(16 /*0x10*/);
      Vector2 vector2 = new Vector2((float) this.EndPosition.x, (float) this.EndPosition.y);
      this.FinalPathPoints.Add(vector2);
      for (int index = 1; index < path.Count; ++index)
      {
        if ((double) this.distanceSqr(path[index], path[index - 1]) >= 64.0 && (double) this.distanceSqr(path[index], this.StartPosition) >= 64.0 && (double) this.distanceSqr(path[index], this.EndPosition) >= 64.0)
        {
          vector2.x = (float) path[index].x;
          vector2.y = (float) path[index].y;
          this.FinalPathPoints.Add(vector2);
        }
      }
      this.FinalPathPoints.Add(new Vector2((float) this.StartPosition.x, (float) this.StartPosition.y));
      this.ProcessPath();
    }
  }

  public void commitPathingMapData()
  {
    for (int index = 0; index < this.FinalPathPoints.Count - 1; ++index)
    {
      Vector2i vector2i;
      vector2i.x = (int) this.FinalPathPoints[index].x;
      vector2i.y = (int) this.FinalPathPoints[index].y;
      PathTile pathTileWorld = this.getPathTileWorld(vector2i);
      if (pathTileWorld != null)
      {
        pathTileWorld.TileState = this.isCountryRoad ? PathTile.PathTileStates.Country : PathTile.PathTileStates.Highway;
        pathTileWorld.PathRadius = (byte) this.radius;
      }
      if (!this.isCountryRoad)
      {
        StreetTile streetTileWorld = this.worldBuilder.GetStreetTileWorld(vector2i);
        if (streetTileWorld != null && !streetTileWorld.ConnectedHighways.Contains(this))
          streetTileWorld.ConnectedHighways.Add(this);
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public PathTile getPathTileWorld(Vector2i worldPosition)
  {
    return this.getPathTileWorld(worldPosition.x, worldPosition.y);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public PathTile getPathTileWorld(int x, int y)
  {
    x /= 10;
    y /= 10;
    if (x < 0 || x >= this.worldBuilder.PathingGrid.GetLength(0))
      return (PathTile) null;
    if (y < 0 || y >= this.worldBuilder.PathingGrid.GetLength(1))
      return (PathTile) null;
    if (this.worldBuilder.PathingGrid[x, y] == null)
      this.worldBuilder.PathingGrid[x, y] = new PathTile();
    return this.worldBuilder.PathingGrid[x, y];
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void roundOffCorners()
  {
    for (int index = 2; index < this.FinalPathPoints.Count - 2; ++index)
    {
      if ((this.FinalPathPoints[index] - this.FinalPathPoints[index - 1]).normalized != (this.FinalPathPoints[index + 1] - this.FinalPathPoints[index]).normalized)
        this.FinalPathPoints[index] = (this.FinalPathPoints[index] + this.FinalPathPoints[index - 1] * 0.5f + this.FinalPathPoints[index + 1] * 0.5f) / 2f;
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void SmoothHeights(float[] heights)
  {
    for (int index = 0; index < this.pathPoints3d.Count; ++index)
      heights[index] = this.pathPoints3d[index].y;
    for (int index = 1; index < this.pathPoints3d.Count - 1; ++index)
    {
      Vector3 vector3 = this.pathPoints3d[index];
      if (this.worldBuilder.poiHeightMask[(int) vector3.x + (int) vector3.z * this.worldBuilder.WorldSize] == (byte) 0)
      {
        vector3.y = (float) (((double) heights[index - 1] + (double) heights[index] + (double) heights[index + 1]) * 0.3333333432674408);
        this.pathPoints3d[index] = vector3;
      }
    }
  }
}
