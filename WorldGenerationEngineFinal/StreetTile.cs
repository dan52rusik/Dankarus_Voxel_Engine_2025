// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.StreetTile
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using UniLinq;
using UnityEngine;

#nullable disable
namespace WorldGenerationEngineFinal;

public class StreetTile
{
  public const int TileSize = 150;
  [PublicizedFrom(EAccessModifier.Private)]
  public const int TileSizeHalf = 75;
  [PublicizedFrom(EAccessModifier.Private)]
  public const float cDensityRadius = 190f;
  [PublicizedFrom(EAccessModifier.Private)]
  public const float cDensityBase = 6f;
  [PublicizedFrom(EAccessModifier.Private)]
  public const float cDensityMid = 20f;
  [PublicizedFrom(EAccessModifier.Private)]
  public const float cDensityMidScale = 1.3f;
  [PublicizedFrom(EAccessModifier.Private)]
  public const float cDensityBudget = 62f;
  [PublicizedFrom(EAccessModifier.Private)]
  public const float cDensityRetry = 18f;
  [PublicizedFrom(EAccessModifier.Private)]
  public const int cRadiationEdgeSize = 1;
  [PublicizedFrom(EAccessModifier.Private)]
  public const float cHeightDiffMax = 20f;
  [PublicizedFrom(EAccessModifier.Private)]
  public const int partDepthLimit = 20;
  [PublicizedFrom(EAccessModifier.Private)]
  public const int poiDepthLimit = 5;
  [PublicizedFrom(EAccessModifier.Private)]
  public const int cTilePadInside = 20;
  [PublicizedFrom(EAccessModifier.Private)]
  public const int cTileSizeInside = 110;
  [PublicizedFrom(EAccessModifier.Private)]
  public const float cSmoothFullRadius = 1.8f;
  [PublicizedFrom(EAccessModifier.Private)]
  public const float cSmoothFadeRadius = 3.2f;
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly WorldBuilder worldBuilder;
  public Township Township;
  public District District;
  public readonly List<Vector2i> UsedExitList = new List<Vector2i>();
  public int ConnectedExits;
  public readonly List<Path> ConnectedHighways = new List<Path>();
  public readonly List<PrefabDataInstance> StreetTilePrefabDatas = new List<PrefabDataInstance>();
  public readonly Vector2i GridPosition;
  public readonly Vector2i WorldPosition;
  public readonly Rect Area;
  public bool OverlapsRadiation;
  public bool OverlapsWater;
  public bool OverlapsBiomes;
  public bool HasSteepSlope;
  public bool AllIsWater;
  public bool HasTrader;
  public bool HasFeature;
  public Vector2i WildernessPOIPos;
  public Vector2i WildernessPOICenter;
  public Vector2i WildernessPOISize;
  public int WildernessPOIHeight;
  [PublicizedFrom(EAccessModifier.Private)]
  public int RoadShape;
  [PublicizedFrom(EAccessModifier.Private)]
  public List<Bounds> partBounds = new List<Bounds>();
  public bool Used;
  [PublicizedFrom(EAccessModifier.Private)]
  public StreetTile.PrefabRotations rotations;
  [PublicizedFrom(EAccessModifier.Private)]
  public TranslationData transData;
  [PublicizedFrom(EAccessModifier.Private)]
  public List<Vector2i> highwayExits = new List<Vector2i>();
  [PublicizedFrom(EAccessModifier.Private)]
  public StreetTile[] neighbors;
  [PublicizedFrom(EAccessModifier.Private)]
  public bool isFullyBlocked;
  [PublicizedFrom(EAccessModifier.Private)]
  public bool isPartBlocked;

  public int GroupID => this.Township == null ? -1 : this.Township.ID;

  public bool IsValidForStreetTile
  {
    get
    {
      return !this.OverlapsBiomes && !this.OverlapsWater && !this.OverlapsRadiation && !this.HasSteepSlope && this.TerrainType != TerrainType.mountains;
    }
  }

  public bool IsValidForGateway
  {
    get
    {
      return !this.OverlapsBiomes && !this.OverlapsWater && !this.OverlapsRadiation && !this.HasSteepSlope && this.TerrainType != TerrainType.mountains && !this.HasPrefabs;
    }
  }

  public bool IsBlocked
  {
    get
    {
      return this.AllIsWater || this.OverlapsWater || this.OverlapsRadiation || this.HasSteepSlope || this.TerrainType == TerrainType.mountains;
    }
  }

  public bool HasPrefabs
  {
    get => this.StreetTilePrefabDatas != null && this.StreetTilePrefabDatas.Count > 0;
  }

  public bool HasStreetTilePrefab
  {
    get => this.Township != null && this.District != null && this.HasPrefabs;
  }

  public Vector2i WorldPositionCenter
  {
    [MethodImpl(MethodImplOptions.AggressiveInlining)] get
    {
      return this.WorldPosition + new Vector2i(75, 75);
    }
  }

  public Vector2i WorldPositionMax
  {
    [MethodImpl(MethodImplOptions.AggressiveInlining)] get
    {
      return this.WorldPosition + new Vector2i(150, 150);
    }
  }

  public BiomeType BiomeType => this.worldBuilder.GetBiome(this.WorldPositionCenter);

  public float PositionHeight => this.worldBuilder.GetHeight(this.WorldPositionCenter);

  public TerrainType TerrainType => this.worldBuilder.GetTerrainType(this.WorldPositionCenter);

  public int RoadExitCount
  {
    get
    {
      int roadExitCount = 0;
      for (int index = 0; index < this.RoadExits.Length; ++index)
      {
        if (this.RoadExits[index])
          ++roadExitCount;
      }
      return roadExitCount;
    }
  }

  public bool[] RoadExits
  {
    get
    {
      return this.worldBuilder.StreetTileShared.RoadShapeExitsPerRotation[this.RoadShape][(int) this.Rotations];
    }
  }

  public string PrefabName => this.worldBuilder.StreetTileShared.RoadShapesDistrict[this.RoadShape];

  public StreetTile.PrefabRotations Rotations
  {
    [MethodImpl(MethodImplOptions.AggressiveInlining)] get => this.rotations;
    set
    {
      if (value == this.rotations && this.transData != null)
        return;
      this.rotations = value;
      if (this.transData != null)
        this.transData.rotation = (int) this.rotations * -90;
      else
        this.transData = new TranslationData(this.WorldPositionCenter.x, this.WorldPositionCenter.y, 1f, (int) this.rotations * -90);
    }
  }

  public Vector2i getHighwayExitPositionByDirection(Vector2i dir)
  {
    for (int index = 0; index < 4; ++index)
    {
      if (dir == this.worldBuilder.StreetTileShared.dir4way[index])
        return this.getHighwayExitPosition(index);
    }
    return Vector2i.zero;
  }

  public Vector2i getHighwayExitPosition(int index)
  {
    if (this.highwayExits.Count == 0)
      this.getAllHighwayExits();
    return this.highwayExits[index];
  }

  public List<Vector2i> getAllHighwayExits()
  {
    if (this.highwayExits.Count == 0)
    {
      this.highwayExits.Add(this.highwayExitFromIndex(0));
      this.highwayExits.Add(this.highwayExitFromIndex(1));
      this.highwayExits.Add(this.highwayExitFromIndex(2));
      this.highwayExits.Add(this.highwayExitFromIndex(3));
    }
    return this.highwayExits;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public Vector2i highwayExitFromIndex(int index)
  {
    switch (index)
    {
      case 0:
        Vector2i vector2i1;
        vector2i1.x = this.WorldPositionCenter.x;
        vector2i1.y = this.WorldPositionMax.y - 1;
        return vector2i1;
      case 1:
        Vector2i vector2i2;
        vector2i2.x = this.WorldPositionMax.x - 1;
        vector2i2.y = this.WorldPositionCenter.y;
        return vector2i2;
      case 2:
        Vector2i vector2i3;
        vector2i3.x = this.WorldPositionCenter.x;
        vector2i3.y = this.WorldPosition.y;
        return vector2i3;
      default:
        Vector2i vector2i4;
        vector2i4.x = this.WorldPosition.x;
        vector2i4.y = this.WorldPositionCenter.y;
        return vector2i4;
    }
  }

  public void SetAllExistingNeighborsForGateway()
  {
    for (int index = 0; index < 4; ++index)
    {
      if (this.Township.Streets.ContainsKey(this.GridPosition + this.worldBuilder.StreetTileShared.dir4way[index]))
        this.SetExitUsed(this.getHighwayExitPosition(index));
    }
  }

  public List<Vector2i> GetHighwayExits(bool isGateway = false)
  {
    List<Vector2i> highwayExits = new List<Vector2i>();
    if (!isGateway)
    {
      for (int index = 0; index < 4; ++index)
      {
        if (this.Township.Streets.ContainsKey(this.GridPosition + this.worldBuilder.StreetTileShared.dir4way[index]))
        {
          this.UsedExitList.Add(this.getHighwayExitPosition(index));
          this.ConnectedExits |= 1 << index;
        }
      }
    }
    if (this.UsedExitList.Count == 1)
    {
      int index1 = -1;
      for (int index2 = 0; index2 < 4; ++index2)
      {
        if ((this.ConnectedExits & 1 << index2) > 0)
        {
          index1 = index2 + 2 & 3;
          break;
        }
      }
      if (index1 != -1)
        highwayExits.Add(this.getHighwayExitPosition(index1));
      else
        Log.Error("Could not find opposite highway exit!");
    }
    else
    {
      for (int index = 0; index < 4; ++index)
      {
        if ((this.ConnectedExits & 1 << index) <= 0 && (isGateway || this.RoadExits[index]))
          highwayExits.Add(this.getHighwayExitPosition(index));
      }
    }
    return highwayExits;
  }

  public List<Vector2i> GetAllHighwayExits()
  {
    List<Vector2i> allHighwayExits = new List<Vector2i>();
    for (int index = 0; index < this.RoadExits.Length; ++index)
      allHighwayExits.Add(this.getHighwayExitPosition(index));
    return allHighwayExits;
  }

  public bool HasExits()
  {
    for (int index = 0; index < this.RoadExits.Length; ++index)
    {
      if (this.RoadExits[index])
        return true;
    }
    return false;
  }

  public int GetExistingExitCount()
  {
    int existingExitCount = 0;
    for (int index = 0; index < this.RoadExits.Length; ++index)
    {
      if (this.RoadExits[index])
        ++existingExitCount;
    }
    return existingExitCount;
  }

  public StreetTile(WorldBuilder _worldBuilder, Vector2i gridPosition)
  {
    this.worldBuilder = _worldBuilder;
    this.GridPosition = gridPosition;
    this.WorldPosition = this.GridPosition * 150;
    this.Area = new Rect(new Vector2((float) this.WorldPosition.x, (float) this.WorldPosition.y), Vector2.one * 150f);
    GameRandom gameRandom = GameRandomManager.Instance.CreateGameRandom(this.worldBuilder.Seed + this.WorldPosition.ToString().GetHashCode());
    this.Rotations = (StreetTile.PrefabRotations) (gameRandom.RandomRange(0, 4) + 1 & 3);
    GameRandomManager.Instance.FreeGameRandom(gameRandom);
    this.RoadShape = 2;
    if (this.GridPosition.x < 1 || this.GridPosition.x >= this.worldBuilder.StreetTileMapSize - 1)
      this.OverlapsRadiation = true;
    if (this.GridPosition.y < 1 || this.GridPosition.y >= this.worldBuilder.StreetTileMapSize - 1)
      this.OverlapsRadiation = true;
    this.UpdateValidity();
  }

  public void UpdateValidity()
  {
    float positionHeight = this.PositionHeight;
    Vector2i worldPositionCenter = this.WorldPositionCenter;
    foreach (Vector2i vector2i1 in this.worldBuilder.StreetTileShared.dir9way)
    {
      Vector2i vector2i2 = worldPositionCenter + vector2i1 * 75;
      if (this.worldBuilder.GetRad(vector2i2.x, vector2i2.y) > (byte) 0)
        this.OverlapsRadiation = true;
      if ((double) Utils.FastAbs(this.worldBuilder.GetHeight(vector2i2.x, vector2i2.y) - positionHeight) > 20.0)
        this.HasSteepSlope = true;
    }
    BiomeType biomeType = this.BiomeType;
    int num1 = 0;
    int num2 = 0;
    Vector2i worldPositionMax = this.WorldPositionMax;
    for (int y = this.WorldPosition.y; y < worldPositionMax.y; y += 3)
    {
      for (int x = this.WorldPosition.x; x < worldPositionMax.x; x += 3)
      {
        ++num1;
        if (biomeType != this.worldBuilder.GetBiome(x, y))
          this.OverlapsBiomes = true;
        if (this.worldBuilder.GetWater(x, y) > (byte) 0)
        {
          ++num2;
          this.OverlapsWater = true;
        }
      }
    }
    if ((double) num2 / (double) num1 <= 0.89999997615814209)
      return;
    this.AllIsWater = true;
  }

  public Stamp[] GetStamps()
  {
    return new Stamp[1]
    {
      new Stamp(this.worldBuilder, this.worldBuilder.StampManager.GetStamp(this.worldBuilder.StreetTileShared.RoadShapes[this.RoadShape]), this.transData, true, new Color(1f, 0.0f, 0.0f, 0.0f))
    };
  }

  public StreetTile[] GetNeighbors()
  {
    if (this.neighbors == null)
    {
      this.neighbors = new StreetTile[4];
      for (int index = 0; index < this.worldBuilder.StreetTileShared.dir4way.Length; ++index)
        this.neighbors[index] = this.GetNeighbor(this.worldBuilder.StreetTileShared.dir4way[index]);
    }
    return this.neighbors;
  }

  public int GetNeighborCount()
  {
    int neighborCount = 0;
    for (int index = 0; index < this.worldBuilder.StreetTileShared.dir4way.Length; ++index)
    {
      if (this.GetNeighbor(this.worldBuilder.StreetTileShared.dir4way[index]) != null)
        ++neighborCount;
    }
    return neighborCount;
  }

  public StreetTile[] GetNeighbors8way()
  {
    if (this.neighbors == null)
    {
      this.neighbors = new StreetTile[8];
      for (int index = 0; index < this.worldBuilder.StreetTileShared.dir8way.Length; ++index)
        this.neighbors[index] = this.GetNeighbor(this.worldBuilder.StreetTileShared.dir8way[index]);
    }
    return this.neighbors;
  }

  public StreetTile GetNeighbor(Vector2i direction)
  {
    return this.worldBuilder.GetStreetTileGrid(this.GridPosition + direction);
  }

  public bool HasNeighbor(StreetTile otherTile)
  {
    foreach (StreetTile neighbor in this.GetNeighbors())
    {
      if (neighbor == otherTile)
        return true;
    }
    return false;
  }

  public StreetTile GetNeighborByIndex(int idx)
  {
    if (this.neighbors == null)
      this.GetNeighbors();
    return idx < 0 || idx >= this.neighbors.Length ? (StreetTile) null : this.neighbors[idx];
  }

  public int GetNeighborIndex(StreetTile otherTile)
  {
    if (this.neighbors == null)
      this.GetNeighbors();
    for (int neighborIndex = 0; neighborIndex < this.neighbors.Length; ++neighborIndex)
    {
      if (this.neighbors[neighborIndex] == otherTile)
        return neighborIndex;
    }
    return -1;
  }

  public bool HasExitTo(StreetTile otherTile)
  {
    int neighborIndex;
    return (this.Township != null || this.District != null) && (otherTile.Township != null || otherTile.District != null) && (neighborIndex = this.GetNeighborIndex(otherTile)) >= 0 && neighborIndex < this.RoadExits.Length && this.RoadExits[neighborIndex];
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public int vectorToRotation(Vector2i direction)
  {
    for (int rotation = 0; rotation < this.worldBuilder.StreetTileShared.dir4way.Length; ++rotation)
    {
      if (this.worldBuilder.StreetTileShared.dir4way[rotation] == direction)
        return rotation;
    }
    return -1;
  }

  public void SetPathingConstraintsForTile(bool allBlocked = false)
  {
    if (this.Township != null && this.District != null)
    {
      this.worldBuilder.PathingUtils.AddFullyBlockedArea(this.Area);
      this.isFullyBlocked = true;
    }
    else if (allBlocked && !this.isFullyBlocked && !this.isPartBlocked)
    {
      this.worldBuilder.PathingUtils.AddFullyBlockedArea(this.Area);
      this.isFullyBlocked = true;
    }
    else
    {
      if (allBlocked)
        return;
      if (this.isFullyBlocked)
        this.worldBuilder.PathingUtils.RemoveFullyBlockedArea(this.Area);
      this.worldBuilder.PathingUtils.AddMoveLimitArea(this.Area);
      this.isPartBlocked = true;
    }
  }

  public void SetRoadExit(int dir, bool value)
  {
    if ((long) (uint) dir >= (long) this.RoadExits.Length)
      return;
    bool[] _exits = (bool[]) this.RoadExits.Clone();
    _exits[dir] = value;
    this.SetRoadExits(_exits);
  }

  public void SetRoadExits(bool _north, bool _east, bool _south, bool _west)
  {
    this.SetRoadExits(new bool[4]
    {
      _north,
      _east,
      _south,
      _west
    });
    this.GetNeighbor(Vector2i.right)?.SetPathingConstraintsForTile(!_east);
    this.GetNeighbor(Vector2i.left)?.SetPathingConstraintsForTile(!_west);
    this.GetNeighbor(Vector2i.up)?.SetPathingConstraintsForTile(!_north);
    this.GetNeighbor(Vector2i.down)?.SetPathingConstraintsForTile(!_south);
  }

  public void SetRoadExits(bool[] _exits)
  {
    StreetTile.PrefabRotations rotations = this.Rotations;
    int roadShape = this.RoadShape;
    for (int index1 = 0; index1 < this.worldBuilder.StreetTileShared.RoadShapeExitCounts.Count; ++index1)
    {
      this.RoadShape = index1;
      for (int index2 = 0; index2 < 4; ++index2)
      {
        this.Rotations = (StreetTile.PrefabRotations) index2;
        if (((IEnumerable<bool>) _exits).SequenceEqual<bool>((IEnumerable<bool>) this.RoadExits))
          return;
      }
    }
    this.Rotations = rotations;
    this.RoadShape = roadShape;
  }

  public bool SetExitUsed(Vector2i exit)
  {
    for (int index = 0; index < this.RoadExits.Length; ++index)
    {
      Vector2i highwayExitPosition = this.getHighwayExitPosition(index);
      if (highwayExitPosition == exit)
      {
        this.SetRoadExit(index, true);
        this.ConnectedExits |= 1 << index;
        if (!this.UsedExitList.Contains(highwayExitPosition))
          this.UsedExitList.Add(highwayExitPosition);
        return true;
      }
    }
    return false;
  }

  public void SetExitUnUsed(Vector2i exit)
  {
    for (int index = 0; index < this.RoadExits.Length; ++index)
    {
      Vector2i highwayExitPosition = this.getHighwayExitPosition(index);
      if (highwayExitPosition == exit)
      {
        this.SetRoadExit(index, false);
        this.ConnectedExits &= ~(1 << index);
        this.UsedExitList.Remove(highwayExitPosition);
        break;
      }
    }
  }

  public bool ContainsHighway => this.ConnectedHighways.Count > 0;

  public bool SpawnPrefabs()
  {
    if (this.District == null || this.District.name == "wilderness")
    {
      if (!this.ContainsHighway)
      {
        this.District = DistrictPlannerStatic.Districts["wilderness"];
        if (this.spawnWildernessPrefab())
          return true;
      }
      this.District = (District) null;
      return false;
    }
    this.spawnStreetTile(this.WorldPosition, string.Format(this.PrefabName, (object) this.District.prefabName), (int) this.Rotations);
    return true;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public bool spawnStreetTile(
    Vector2i tileMinPositionWorld,
    string streetPrefabName,
    int baseRotations)
  {
    bool useExactString = false;
    PrefabData streetTile = this.worldBuilder.PrefabManager.GetStreetTile(streetPrefabName, this.WorldPositionCenter, useExactString);
    if (streetTile == null && string.Format(this.PrefabName, (object) "") != streetPrefabName)
      streetTile = this.worldBuilder.PrefabManager.GetStreetTile(string.Format(this.PrefabName, (object) ""), this.WorldPositionCenter, useExactString);
    if (streetTile == null)
      return false;
    int _y = this.Township.Height + streetTile.yOffset;
    if (_y < 3)
      return false;
    int num1 = baseRotations + (int) streetTile.RotationsToNorth & 3;
    switch (num1)
    {
      case 1:
        num1 = 3;
        break;
      case 3:
        num1 = 1;
        break;
    }
    Vector3i _position = new Vector3i(tileMinPositionWorld.x, _y, tileMinPositionWorld.y) + this.worldBuilder.PrefabWorldOffset;
    int num2;
    if (this.worldBuilder.PrefabManager.StreetTilesUsed.TryGetValue(streetTile.Name, out num2))
      this.worldBuilder.PrefabManager.StreetTilesUsed[streetTile.Name] = num2 + 1;
    else
      this.worldBuilder.PrefabManager.StreetTilesUsed.Add(streetTile.Name, 1);
    float totalDensityLeft = 62f;
    float num3;
    if (PrefabManagerStatic.TileMaxDensityScore.TryGetValue(streetTile.Name, out num3))
      totalDensityLeft = num3;
    this.AddPrefab(new PrefabDataInstance(this.worldBuilder.PrefabManager.PrefabInstanceId++, _position, (byte) num1, streetTile));
    this.SpawnMarkerPartsAndPrefabs(streetTile, new Vector3i(this.WorldPosition.x, _y, this.WorldPosition.y), num1, 0, totalDensityLeft);
    return true;
  }

  public void SmoothWildernessTerrain()
  {
    this.SmoothTerrainRect(this.WildernessPOIPos, this.WildernessPOISize.x, this.WildernessPOISize.y, this.WildernessPOIHeight, 18);
  }

  public void SmoothTownshipTerrain()
  {
    if (this.Township == null || this.District == null)
      return;
    this.Township.CalcCenterStreetTile();
    this.SmoothTerrainRect(this.WorldPosition, 150, 150, this.Township.Height, this.Township.Streets.Count <= 2 ? 50 : 110);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void SmoothTerrainRect(
    Vector2i _startPos,
    int _sizeX,
    int _sizeY,
    int _height,
    int _fadeRange)
  {
    int num1 = _fadeRange + 1;
    float num2 = (float) _fadeRange;
    int x = _startPos.x;
    int num3 = _startPos.x + _sizeX;
    int y = _startPos.y;
    int num4 = _startPos.y + _sizeY;
    int num5 = Utils.FastMax(x - num1, 1);
    int num6 = Utils.FastMax(y - num1, 1);
    int num7 = Utils.FastMin(num3 + num1, this.worldBuilder.WorldSize);
    int num8 = Utils.FastMin(num4 + num1, this.worldBuilder.WorldSize);
    for (int index1 = num6; index1 < num8; ++index1)
    {
      bool flag1 = index1 >= y && index1 <= num4;
      int y2 = flag1 ? index1 : (index1 < _startPos.y ? y : num4);
      for (int index2 = num5; index2 < num7; ++index2)
      {
        bool flag2 = index2 >= x && index2 <= num3;
        if (flag2 & flag1)
        {
          this.worldBuilder.SetHeightTrusted(index2, index1, (float) _height);
        }
        else
        {
          int x2 = flag2 ? index2 : (index2 < _startPos.x ? x : num3);
          float t = Mathf.Sqrt((float) this.distanceSqr(index2, index1, x2, y2)) / num2;
          if ((double) t < 1.0)
          {
            float height = this.worldBuilder.GetHeight(index2, index1);
            this.worldBuilder.SetHeightTrusted(index2, index1, StreetTile.SmoothStep((float) _height, height, (double) t));
          }
        }
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void SmoothTerrainCircle(Vector2i _centerPos, int _size, int _height)
  {
    int num1;
    float num2 = (float) (num1 = _size / 2) * 1.8f;
    int num3 = Mathf.CeilToInt(num2 * num2);
    int num4 = (int) ((double) num1 * 3.2000000476837158);
    float num5 = (float) num4 - num2;
    int num6 = Utils.FastMax(_centerPos.x - num4, 1);
    int num7 = Utils.FastMax(_centerPos.y - num4, 1);
    int num8 = Utils.FastMin(_centerPos.x + num4, this.worldBuilder.WorldSize);
    int num9 = Utils.FastMin(_centerPos.y + num4, this.worldBuilder.WorldSize);
    for (int index1 = num7; index1 < num9; ++index1)
    {
      for (int index2 = num6; index2 < num8; ++index2)
      {
        int f = this.distanceSqr(index2, index1, _centerPos.x, _centerPos.y);
        if (f <= num3)
        {
          this.worldBuilder.SetHeightTrusted(index2, index1, (float) _height);
        }
        else
        {
          float t = (Mathf.Sqrt((float) f) - num2) / num5;
          if ((double) t < 1.0)
          {
            float height = this.worldBuilder.GetHeight(index2, index1);
            this.worldBuilder.SetHeightTrusted(index2, index1, StreetTile.SmoothStep((float) _height, height, (double) t));
          }
        }
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static float SmoothStep(float from, float to, double t)
  {
    t = -2.0 * t * t * t + 3.0 * t * t;
    return (float) ((double) to * t + (double) from * (1.0 - t));
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public bool spawnWildernessPrefab()
  {
    GameRandom gameRandom = GameRandomManager.Instance.CreateGameRandom(this.worldBuilder.Seed + 4096953 + this.GridPosition.x + this.GridPosition.y * 200);
    FastTags<TagGroup.Poi> fastTags = this.worldBuilder.Towns == WorldBuilder.GenerationSelections.None ? FastTags<TagGroup.Poi>.none : this.worldBuilder.StreetTileShared.traderTag;
    PrefabManager prefabManager = this.worldBuilder.PrefabManager;
    FastTags<TagGroup.Poi> _withoutTags = fastTags;
    FastTags<TagGroup.Poi> none = FastTags<TagGroup.Poi>.none;
    Vector2i worldPositionCenter = this.WorldPositionCenter;
    Vector2i minSize = new Vector2i();
    Vector2i maxSize = new Vector2i();
    Vector2i center = worldPositionCenter;
    PrefabData wildernessPrefab = prefabManager.GetWildernessPrefab(_withoutTags, none, minSize, maxSize, center);
    for (int index = 0; index < 6; ++index)
    {
      if (this.spawnWildernessPrefab(wildernessPrefab, gameRandom))
      {
        GameRandomManager.Instance.FreeGameRandom(gameRandom);
        return true;
      }
    }
    GameRandomManager.Instance.FreeGameRandom(gameRandom);
    return false;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public bool spawnWildernessPrefab(PrefabData prefab, GameRandom rndm)
  {
    int num1 = (int) prefab.RotationsToNorth + rndm.RandomRange(0, 4) & 3;
    int num2 = prefab.size.x;
    int num3 = prefab.size.z;
    if (num1 == 1 || num1 == 3)
    {
      num2 = prefab.size.z;
      num3 = prefab.size.x;
    }
    Vector2i vector2i1;
    if (num2 >= 110 || num3 >= 110)
    {
      vector2i1 = this.WorldPositionCenter - new Vector2i(num2 / 2, num3 / 2);
      if (num2 > 150 || num3 > 150)
        Log.Warning("RWG spawnWildernessPrefab {0}, overflows TileSize {1}", (object) prefab.Name, (object) 150);
    }
    else
    {
      vector2i1.x = this.WorldPosition.x + 20 + rndm.RandomRange(110 - num2);
      vector2i1.y = this.WorldPosition.y + 20 + rndm.RandomRange(110 - num3);
    }
    if (vector2i1.x < 0 || vector2i1.x + num2 > this.worldBuilder.WorldSize || vector2i1.y < 0 || vector2i1.y + num3 > this.worldBuilder.WorldSize)
      return false;
    Vector2i vector2i2;
    vector2i2.x = vector2i1.x + num2 / 2;
    vector2i2.y = vector2i1.y + num3 / 2;
    Vector2i vector2i3;
    vector2i3.x = vector2i1.x + num2 - 1;
    vector2i3.y = vector2i1.y + num3 - 1;
    BiomeType biome = this.worldBuilder.GetBiome(vector2i2.x, vector2i2.y);
    int num4 = Mathf.CeilToInt(this.worldBuilder.GetHeight(vector2i2.x, vector2i2.y));
    List<int> heights = new List<int>();
    for (int y = vector2i1.y; y < vector2i1.y + num3; ++y)
    {
      for (int x = vector2i1.x; x < vector2i1.x + num2; ++x)
      {
        if (this.worldBuilder.GetWater(x, y) > (byte) 0 || biome != this.worldBuilder.GetBiome(x, y))
          return false;
        int num5 = Mathf.CeilToInt(this.worldBuilder.GetHeight(x, y));
        if (Utils.FastAbsInt(num5 - num4) > 11)
          return false;
        heights.Add(num5);
      }
    }
    int medianHeight = this.getMedianHeight(heights);
    if (medianHeight + prefab.yOffset < 2)
      return false;
    int heightCeil = this.getHeightCeil((float) vector2i2.x, (float) vector2i2.y);
    Vector3i vector3i = new Vector3i(this.subHalfWorld(vector2i1.x), heightCeil, this.subHalfWorld(vector2i1.y));
    int _id = this.worldBuilder.PrefabManager.PrefabInstanceId++;
    rndm.SetSeed(vector2i1.x + vector2i1.x * vector2i1.y + vector2i1.y);
    if (prefab.POIMarkers != null)
    {
      List<Prefab.Marker> markerList = prefab.RotatePOIMarkers(true, num1);
      for (int index = markerList.Count - 1; index >= 0; --index)
      {
        if (markerList[index].MarkerType != Prefab.Marker.MarkerTypes.RoadExit)
          markerList.RemoveAt(index);
      }
      if (markerList.Count > 0)
      {
        int index = rndm.RandomRange(0, markerList.Count);
        float _pathRadius = (float) Utils.FastMax(markerList[index].Size.x, markerList[index].Size.z) * 0.5f;
        Vector3i start = markerList[index].Start;
        Vector2 vector2_1 = new Vector2((float) start.x + (float) markerList[index].Size.x / 2f, (float) start.z + (float) markerList[index].Size.z / 2f);
        Vector2 vector2_2 = new Vector2((float) vector2i1.x + vector2_1.x, (float) vector2i1.y + vector2_1.y);
        this.worldBuilder.WildernessPlanner.WildernessPathInfos.Add(new WorldBuilder.WildernessPathInfo(new Vector2i(vector2_2), _id, _pathRadius, this.worldBuilder.GetBiome((int) vector2_2.x, (int) vector2_2.y)));
      }
    }
    int _y = medianHeight + prefab.yOffset;
    this.SpawnMarkerPartsAndPrefabsWilderness(prefab, new Vector3i(vector2i1.x, _y, vector2i1.y), (int) (byte) num1);
    this.AddPrefab(new PrefabDataInstance(_id, new Vector3i(vector3i.x, _y, vector3i.z), (byte) num1, prefab));
    ++this.worldBuilder.WildernessPrefabCount;
    this.WildernessPOIPos = vector2i1;
    this.WildernessPOICenter = vector2i2;
    this.WildernessPOISize.x = num2;
    this.WildernessPOISize.y = num3;
    this.WildernessPOIHeight = medianHeight;
    int num6 = Mathf.FloorToInt((float) vector2i1.x / 10f) - 2;
    int num7 = Mathf.CeilToInt((float) (((double) vector2i3.x + 0.5) / 10.0)) + 2;
    int num8 = Mathf.FloorToInt((float) vector2i1.y / 10f) - 2;
    int num9 = Mathf.CeilToInt((float) (((double) vector2i3.y + 0.5) / 10.0)) + 2;
    int num10 = num6 + 2;
    int num11 = num7 - 2 - 1;
    int num12 = num8 + 2;
    int num13 = num9 - 2 - 1;
    for (int y = num8; y < num9; ++y)
    {
      if (y >= 0 && y < this.worldBuilder.PathingGrid.GetLength(1))
      {
        for (int x = num6; x < num7; ++x)
        {
          if (x >= 0 && x < this.worldBuilder.PathingGrid.GetLength(0))
          {
            if (x >= num10 && x <= num11 && y >= num12 && y <= num13)
              this.worldBuilder.PathingUtils.SetPathBlocked(x, y, true);
            else if (x == num6 || x == num7 - 1 || y == num8 || y == num9 - 1)
              this.worldBuilder.PathingUtils.SetPathBlocked(x, y, (sbyte) 2);
            else
              this.worldBuilder.PathingUtils.SetPathBlocked(x, y, (sbyte) 3);
          }
        }
      }
    }
    int num14 = Mathf.FloorToInt((float) vector2i1.x) - 1;
    int num15 = Mathf.CeilToInt((float) vector2i3.x + 0.5f) + 1;
    int num16 = Mathf.FloorToInt((float) vector2i1.y) - 1;
    int num17 = Mathf.CeilToInt((float) vector2i3.y + 0.5f) + 1;
    for (int x = num14; x < num15; x += 150)
    {
      for (int y = num16; y < num17; y += 150)
      {
        StreetTile streetTileWorld = this.worldBuilder.GetStreetTileWorld(x, y);
        if (streetTileWorld != null)
          streetTileWorld.Used = true;
      }
    }
    return true;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void AddPrefab(PrefabDataInstance pdi)
  {
    this.StreetTilePrefabDatas.Add(pdi);
    if (this.Township != null)
      this.Township.AddPrefab(pdi);
    else
      this.worldBuilder.PrefabManager.AddUsedPrefabWorld(-1, pdi);
  }

  public bool NeedsWildernessSmoothing => this.WildernessPOISize.x > 0;

  [PublicizedFrom(EAccessModifier.Private)]
  public int getMedianHeight(List<int> heights)
  {
    heights.Sort();
    int count = heights.Count;
    int index = count / 2;
    return count % 2 == 0 ? (heights[index] + heights[index - 1]) / 2 : heights[index];
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public int getAverageHeight(List<int> heights)
  {
    int num = 0;
    foreach (int height in heights)
      num += height;
    return num / heights.Count;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public int getHeightCeil(float x, float y) => Mathf.CeilToInt(this.worldBuilder.GetHeight(x, y));

  [PublicizedFrom(EAccessModifier.Private)]
  public int subHalfWorld(int pos) => pos - this.worldBuilder.WorldSize / 2;

  [PublicizedFrom(EAccessModifier.Private)]
  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public int distanceSqr(Vector2i v1, Vector2i v2)
  {
    int num1 = v1.x - v2.x;
    int num2 = v1.y - v2.y;
    return num1 * num1 + num2 * num2;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public int distanceSqr(int x1, int y1, int x2, int y2)
  {
    int num1 = x1 - x2;
    int num2 = y1 - y2;
    return num1 * num1 + num2 * num2;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public float distSqr(Vector2 v1, Vector2 v2)
  {
    double num1 = (double) v1.x - (double) v2.x;
    float num2 = v1.y - v2.y;
    return (float) (num1 * num1 + (double) num2 * (double) num2);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void SpawnMarkerPartsAndPrefabs(
    PrefabData _parentPrefab,
    Vector3i _parentPosition,
    int _parentRotations,
    int _depth,
    float totalDensityLeft)
  {
    List<Prefab.Marker> markerList = _parentPrefab.RotatePOIMarkers(true, _parentRotations);
    if (markerList.Count == 0)
      return;
    FastTags<TagGroup.Poi> fastTags = FastTags<TagGroup.Poi>.Parse(this.District.name);
    this.worldBuilder.PathingUtils.AddFullyBlockedArea(this.Area);
    Vector3i size = _parentPrefab.size;
    if (_parentRotations % 2 == 1)
    {
      ref int local1 = ref size.z;
      ref int local2 = ref size.x;
      int x = size.x;
      int z = size.z;
      local1 = x;
      int num = z;
      local2 = num;
    }
    List<Prefab.Marker> all1 = markerList.FindAll((Predicate<Prefab.Marker>) ([PublicizedFrom(EAccessModifier.Internal)] (m) => m.MarkerType == Prefab.Marker.MarkerTypes.POISpawn));
    if (_depth < 5 && all1.Count > 0)
    {
      all1.Sort((Comparison<Prefab.Marker>) ([PublicizedFrom(EAccessModifier.Internal)] (m1, m2) => (m2.Size.x + m2.Size.y + m2.Size.z).CompareTo(m1.Size.x + m1.Size.y + m1.Size.z)));
      List<string> stringList = new List<string>();
      for (int index = 0; index < all1.Count; ++index)
      {
        if (!stringList.Contains(all1[index].GroupName))
          stringList.Add(all1[index].GroupName);
      }
      this.Township.rand.SetSeed(this.Township.ID + (_parentPosition.x * _parentPosition.x + _parentPosition.y * _parentPosition.y));
      foreach (string str in stringList)
      {
        string groupName = str;
        List<Prefab.Marker> list = all1.Where<Prefab.Marker>((Func<Prefab.Marker, bool>) ([PublicizedFrom(EAccessModifier.Internal)] (m) => m.GroupName == groupName)).OrderByDescending<Prefab.Marker, float>((Func<Prefab.Marker, float>) ([PublicizedFrom(EAccessModifier.Private)] (m) => this.Township.rand.RandomFloat)).ToList<Prefab.Marker>();
        for (int index1 = 0; index1 < list.Count; ++index1)
        {
          Prefab.Marker marker = list[index1];
          Vector2i maxSize = new Vector2i(marker.Size.x, marker.Size.z);
          Vector2i vector2i1 = new Vector2i(marker.Start.x, marker.Start.z);
          Vector2i vector2i2 = vector2i1 + maxSize;
          Vector2i vector2i3 = vector2i1 + maxSize / 2;
          Vector2i minSize = maxSize;
          if (this.District.spawnCustomSizePrefabs)
          {
            int num;
            if (this.District.name != "gateway" && (num = Prefab.Marker.MarkerSizes.IndexOf(new Vector3i(maxSize.x, 0, maxSize.y))) >= 0)
            {
              if (num > 0)
                minSize = new Vector2i(Prefab.Marker.MarkerSizes[num - 1].x + 1, Prefab.Marker.MarkerSizes[num - 1].z + 1);
            }
            else
              minSize = maxSize / 2;
          }
          Vector2i center = new Vector2i(_parentPosition.x + vector2i3.x, _parentPosition.z + vector2i3.y);
          if (_depth == 0)
          {
            int halfWorldSize = this.worldBuilder.HalfWorldSize;
            Vector2 a = new Vector2((float) (center.x - halfWorldSize), (float) (center.y - halfWorldSize));
            float num = 0.0f;
            List<PrefabDataInstance> prefabs = this.Township.Prefabs;
            for (int index2 = 0; index2 < prefabs.Count; ++index2)
            {
              PrefabDataInstance prefabDataInstance = prefabs[index2];
              float densityScore = prefabDataInstance.prefab.DensityScore;
              if ((double) densityScore > 6.0)
              {
                Vector2 centerXzV2 = prefabDataInstance.CenterXZV2;
                if ((double) Vector2.Distance(a, centerXzV2) < 190.0)
                {
                  if ((double) densityScore >= 20.0)
                    num += densityScore * 1.3f;
                  else
                    num += densityScore - 6f;
                }
              }
            }
            if ((double) num > 0.0)
              totalDensityLeft = Utils.FastMax(6f, totalDensityLeft - num);
          }
          PrefabData prefabWithDistrict = this.worldBuilder.PrefabManager.GetPrefabWithDistrict(this.District, marker.Tags, minSize, maxSize, center, totalDensityLeft, 1f);
          if (prefabWithDistrict == null)
          {
            prefabWithDistrict = this.worldBuilder.PrefabManager.GetPrefabWithDistrict(this.District, marker.Tags, minSize, maxSize, center, totalDensityLeft + 8f, 0.3f);
            if (prefabWithDistrict == null)
            {
              prefabWithDistrict = this.worldBuilder.PrefabManager.GetPrefabWithDistrict(this.District, marker.Tags, minSize, maxSize, center, 18f, 0.0f);
              if (prefabWithDistrict == null)
              {
                Log.Warning("SpawnMarkerPartsAndPrefabs failed {0}, tags {1}, size {2} {3}, totalDensityLeft {4}", (object) this.District.name, (object) marker.Tags, (object) minSize, (object) maxSize, (object) totalDensityLeft);
                continue;
              }
              Log.Warning("SpawnMarkerPartsAndPrefabs retry2 {0}, tags {1}, size {2} {3}, totalDensityLeft {4}, picked {5}, density {6}", (object) this.District.name, (object) marker.Tags, (object) minSize, (object) maxSize, (object) totalDensityLeft, (object) prefabWithDistrict.Name, (object) prefabWithDistrict.DensityScore);
            }
          }
          int _x = _parentPosition.x + marker.Start.x;
          int _z = _parentPosition.z + marker.Start.z;
          if (_parentPosition.y + marker.Start.y + prefabWithDistrict.yOffset < 3)
          {
            Log.Error("SpawnMarkerPartsAndPrefabs y low! {0}, pos {1} {2}", (object) prefabWithDistrict.Name, (object) _x, (object) _z);
          }
          else
          {
            totalDensityLeft -= prefabWithDistrict.DensityScore;
            if (prefabWithDistrict.Tags.Test_AnySet(this.worldBuilder.StreetTileShared.traderTag) || prefabWithDistrict.Name.Contains("trader"))
            {
              Vector2i vector2i4;
              vector2i4.x = _x + marker.Size.x / 2;
              vector2i4.y = _z + marker.Size.z / 2;
              this.worldBuilder.TraderCenterPositions.Add(vector2i4);
              if (this.BiomeType == BiomeType.forest)
                this.worldBuilder.TraderForestCenterPositions.Add(vector2i4);
              this.HasTrader = true;
              Log.Out("Trader {0}, {1}, {2}, marker {3}, at {4}", (object) prefabWithDistrict.Name, (object) this.BiomeType, (object) this.District.name, (object) marker.Name, (object) vector2i4);
            }
            int rotations = (int) marker.Rotations;
            byte num1 = (byte) (_parentRotations + (int) prefabWithDistrict.RotationsToNorth + rotations & 3);
            int num2 = prefabWithDistrict.size.x;
            int num3 = prefabWithDistrict.size.z;
            if (num1 == (byte) 1 || num1 == (byte) 3)
            {
              int num4 = num2;
              int num5 = num3;
              num3 = num4;
              num2 = num5;
            }
            switch (rotations)
            {
              case 0:
                _x += maxSize.x / 2 - num2 / 2;
                _z = _z + maxSize.y - num3;
                break;
              case 1:
                _z += maxSize.y / 2 - num3 / 2;
                break;
              case 2:
                _x += maxSize.x / 2 - num2 / 2;
                break;
              case 3:
                _z += maxSize.y / 2 - num3 / 2;
                _x = _x + maxSize.x - num2;
                break;
            }
            PrefabDataInstance pdi = new PrefabDataInstance(this.worldBuilder.PrefabManager.PrefabInstanceId++, new Vector3i(_x, _parentPosition.y + marker.Start.y + prefabWithDistrict.yOffset, _z) + this.worldBuilder.PrefabWorldOffset, num1, prefabWithDistrict);
            Color color = this.District.preview_color;
            if (pdi.prefab.Name.StartsWith("remnant_") || pdi.prefab.Name.StartsWith("abandoned_"))
            {
              color.r *= 0.75f;
              color.g *= 0.75f;
              color.b *= 0.75f;
            }
            else if ((double) pdi.prefab.DensityScore < 1.0)
            {
              color.r *= 0.4f;
              color.g *= 0.4f;
              color.b *= 0.4f;
            }
            else if (pdi.prefab.Name.StartsWith("trader_"))
              color = new Color(0.6f, 0.3f, 0.3f);
            pdi.previewColor = (Color32) color;
            this.Township.AddPrefab(pdi);
            this.SpawnMarkerPartsAndPrefabs(prefabWithDistrict, new Vector3i(_x, _parentPosition.y + marker.Start.y + prefabWithDistrict.yOffset, _z), (int) num1, _depth + 1, totalDensityLeft);
            break;
          }
        }
      }
    }
    List<Prefab.Marker> all2 = markerList.FindAll((Predicate<Prefab.Marker>) ([PublicizedFrom(EAccessModifier.Internal)] (m) => m.MarkerType == Prefab.Marker.MarkerTypes.PartSpawn));
    if (_depth < 20 && all2.Count > 0)
    {
      List<string> stringList = new List<string>();
      for (int index = 0; index < all2.Count; ++index)
      {
        if (!stringList.Contains(all2[index].GroupName))
          stringList.Add(all2[index].GroupName);
      }
      this.Township.rand.SetSeed(this.Township.ID + (_parentPosition.x * _parentPosition.x + _parentPosition.y * _parentPosition.y) + 1);
      foreach (string str in stringList)
      {
        string groupName = str;
        List<Prefab.Marker> list = all2.Where<Prefab.Marker>((Func<Prefab.Marker, bool>) ([PublicizedFrom(EAccessModifier.Internal)] (m) => m.GroupName == groupName)).OrderByDescending<Prefab.Marker, float>((Func<Prefab.Marker, float>) ([PublicizedFrom(EAccessModifier.Private)] (m) => this.Township.rand.RandomFloat)).ToList<Prefab.Marker>();
        float num6 = 1f;
        if (list.Count > 1)
        {
          num6 = 0.0f;
          foreach (Prefab.Marker marker in list)
            num6 += marker.PartChanceToSpawn;
        }
        float num7 = 0.0f;
        using (List<Prefab.Marker>.Enumerator enumerator = list.GetEnumerator())
        {
label_93:
          while (enumerator.MoveNext())
          {
            Prefab.Marker current = enumerator.Current;
            num7 += current.PartChanceToSpawn / num6;
            if ((double) this.Township.rand.RandomRange(0.0f, 1f) <= (double) num7)
            {
              if (!current.Tags.IsEmpty)
              {
                if (_depth == 0)
                {
                  if (!this.District.tag.Test_AnySet(current.Tags))
                    continue;
                }
                else if (!current.Tags.IsEmpty && !fastTags.Test_AnySet(current.Tags))
                  continue;
              }
              PrefabData prefabByName = this.worldBuilder.PrefabManager.GetPrefabByName(current.PartToSpawn);
              if (prefabByName == null)
              {
                Log.Error("Part to spawn {0} not found!", (object) current.PartToSpawn);
              }
              else
              {
                Vector3i _position = new Vector3i(_parentPosition.x + current.Start.x - this.worldBuilder.WorldSize / 2, _parentPosition.y + current.Start.y, _parentPosition.z + current.Start.z - this.worldBuilder.WorldSize / 2);
                if (_position.y > 0)
                {
                  byte num8 = current.Rotations;
                  switch (num8)
                  {
                    case 1:
                      num8 = (byte) 3;
                      break;
                    case 3:
                      num8 = (byte) 1;
                      break;
                  }
                  byte num9 = (byte) ((_parentRotations + (int) prefabByName.RotationsToNorth + (int) num8) % 4);
                  Vector3i vector3i = prefabByName.size;
                  if (num9 == (byte) 1 || num9 == (byte) 3)
                    vector3i = new Vector3i(vector3i.z, vector3i.y, vector3i.x);
                  Bounds bounds = new Bounds((Vector3) (_position + vector3i * 0.5f), (Vector3) vector3i - Vector3.one);
                  foreach (Bounds partBound in this.partBounds)
                  {
                    if (partBound.Intersects(bounds))
                      goto label_93;
                  }
                  this.Township.AddPrefab(new PrefabDataInstance(this.worldBuilder.PrefabManager.PrefabInstanceId++, _position, num9, prefabByName));
                  totalDensityLeft -= prefabByName.DensityScore;
                  this.partBounds.Add(bounds);
                  this.SpawnMarkerPartsAndPrefabs(prefabByName, _parentPosition + current.Start, (int) num9, _depth + 1, totalDensityLeft);
                  break;
                }
              }
            }
          }
        }
      }
    }
    if (this.District == null || !(this.District.name == "gateway"))
      return;
    List<Prefab.Marker> all3 = markerList.FindAll((Predicate<Prefab.Marker>) ([PublicizedFrom(EAccessModifier.Internal)] (m) => m.PartToSpawn.Contains("highway_transition")));
    if (all3.Count <= 0)
      return;
    foreach (Prefab.Marker marker in all3)
    {
      Vector2 vector2 = new Vector2((float) marker.Start.x, (float) marker.Start.z) - new Vector2((float) (size.x / 2), (float) (size.z / 2));
      if ((double) Mathf.Abs(vector2.x) > (double) Mathf.Abs(vector2.y))
      {
        if ((double) vector2.x > 0.0)
        {
          if (!this.HasExitTo(this.GetNeighbor(Vector2i.right)) || this.GetNeighbor(Vector2i.right).Township != this.Township)
            continue;
        }
        else if (!this.HasExitTo(this.GetNeighbor(Vector2i.left)) || this.GetNeighbor(Vector2i.left).Township != this.Township)
          continue;
      }
      else if ((double) vector2.y > 0.0)
      {
        if (!this.HasExitTo(this.GetNeighbor(Vector2i.up)) || this.GetNeighbor(Vector2i.up).Township != this.Township)
          continue;
      }
      else if (!this.HasExitTo(this.GetNeighbor(Vector2i.down)) || this.GetNeighbor(Vector2i.down).Township != this.Township)
        continue;
      PrefabData prefabByName = this.worldBuilder.PrefabManager.GetPrefabByName(marker.PartToSpawn);
      if (prefabByName != null)
      {
        Vector3i _position = new Vector3i(_parentPosition.x + marker.Start.x - this.worldBuilder.WorldSize / 2, _parentPosition.y + marker.Start.y, _parentPosition.z + marker.Start.z - this.worldBuilder.WorldSize / 2);
        if (_position.y > 0)
        {
          byte num = marker.Rotations;
          switch (num)
          {
            case 1:
              num = (byte) 3;
              break;
            case 3:
              num = (byte) 1;
              break;
          }
          byte _rotation = (byte) ((_parentRotations + (int) prefabByName.RotationsToNorth + (int) num) % 4);
          Vector3i vector3i = prefabByName.size;
          if (_rotation == (byte) 1 || _rotation == (byte) 3)
            vector3i = new Vector3i(vector3i.z, vector3i.y, vector3i.x);
          this.Township.AddPrefab(new PrefabDataInstance(this.worldBuilder.PrefabManager.PrefabInstanceId++, _position, _rotation, prefabByName));
        }
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void SpawnMarkerPartsAndPrefabsWilderness(
    PrefabData _parentPrefab,
    Vector3i _parentPosition,
    int _parentRotations)
  {
    GameRandom gameRandom = GameRandomManager.Instance.CreateGameRandom(_parentPosition.ToString().GetHashCode());
    List<Prefab.Marker> markerList = _parentPrefab.RotatePOIMarkers(true, _parentRotations);
    List<Prefab.Marker> all1 = markerList.FindAll((Predicate<Prefab.Marker>) ([PublicizedFrom(EAccessModifier.Internal)] (m) => m.MarkerType == Prefab.Marker.MarkerTypes.POISpawn));
    if (all1.Count > 0)
    {
      for (int index = 0; index < all1.Count; ++index)
      {
        Prefab.Marker marker = all1[index];
        Vector2i maxSize = new Vector2i(marker.Size.x, marker.Size.z);
        Vector2i vector2i = new Vector2i(marker.Start.x, marker.Start.z) + maxSize / 2;
        Vector2i minSize = maxSize;
        PrefabData wildernessPrefab = this.worldBuilder.PrefabManager.GetWildernessPrefab(this.worldBuilder.StreetTileShared.traderTag, marker.Tags, minSize, maxSize, new Vector2i(_parentPosition.x + vector2i.x, _parentPosition.z + vector2i.y));
        if (wildernessPrefab != null)
        {
          int _x = _parentPosition.x + marker.Start.x;
          int _z = _parentPosition.z + marker.Start.z;
          int rotations = (int) marker.Rotations;
          byte num1 = (byte) (_parentRotations + (int) wildernessPrefab.RotationsToNorth + rotations & 3);
          int num2 = wildernessPrefab.size.x;
          int num3 = wildernessPrefab.size.z;
          if (num1 == (byte) 1 || num1 == (byte) 3)
          {
            int num4 = num2;
            num2 = num3;
            num3 = num4;
          }
          switch (rotations)
          {
            case 0:
              _x += maxSize.x / 2 - num2 / 2;
              _z = _z + maxSize.y - num3;
              break;
            case 1:
              _z += maxSize.y / 2 - num3 / 2;
              break;
            case 2:
              _x += maxSize.x / 2 - num2 / 2;
              break;
            case 3:
              _z += maxSize.y / 2 - num3 / 2;
              _x = _x + maxSize.x - num2;
              break;
          }
          this.AddPrefab(new PrefabDataInstance(this.worldBuilder.PrefabManager.PrefabInstanceId++, new Vector3i(_x - this.worldBuilder.WorldSize / 2, _parentPosition.y + marker.Start.y + wildernessPrefab.yOffset, _z - this.worldBuilder.WorldSize / 2), num1, wildernessPrefab));
          ++this.worldBuilder.WildernessPrefabCount;
          wildernessPrefab.RotatePOIMarkers(true, (int) num1);
          this.SpawnMarkerPartsAndPrefabsWilderness(wildernessPrefab, new Vector3i(_x, _parentPosition.y + marker.Start.y + wildernessPrefab.yOffset, _z), (int) num1);
        }
      }
    }
    List<Prefab.Marker> all2 = markerList.FindAll((Predicate<Prefab.Marker>) ([PublicizedFrom(EAccessModifier.Internal)] (m) => m.MarkerType == Prefab.Marker.MarkerTypes.PartSpawn));
    if (all2.Count > 0)
    {
      List<string> stringList = new List<string>();
      for (int index = 0; index < all2.Count; ++index)
      {
        if (!stringList.Contains(all2[index].GroupName))
          stringList.Add(all2[index].GroupName);
      }
      foreach (string str in stringList)
      {
        string groupName = str;
        List<Prefab.Marker> all3 = all2.FindAll((Predicate<Prefab.Marker>) ([PublicizedFrom(EAccessModifier.Internal)] (m) => m.GroupName == groupName));
        float num5 = 1f;
        if (all3.Count > 1)
        {
          num5 = 0.0f;
          foreach (Prefab.Marker marker in all3)
            num5 += marker.PartChanceToSpawn;
        }
        float num6 = 0.0f;
        foreach (Prefab.Marker marker in all3)
        {
          num6 += marker.PartChanceToSpawn / num5;
          if ((double) gameRandom.RandomRange(0.0f, 1f) <= (double) num6 && (marker.Tags.IsEmpty || this.worldBuilder.StreetTileShared.wildernessTag.Test_AnySet(marker.Tags)))
          {
            PrefabData prefabByName = this.worldBuilder.PrefabManager.GetPrefabByName(marker.PartToSpawn);
            if (prefabByName == null)
            {
              Log.Error("Part to spawn {0} not found!", (object) marker.PartToSpawn);
            }
            else
            {
              Vector3i _position = new Vector3i(_parentPosition.x + marker.Start.x - this.worldBuilder.WorldSize / 2, _parentPosition.y + marker.Start.y, _parentPosition.z + marker.Start.z - this.worldBuilder.WorldSize / 2);
              byte num7 = marker.Rotations;
              switch (num7)
              {
                case 1:
                  num7 = (byte) 3;
                  break;
                case 3:
                  num7 = (byte) 1;
                  break;
              }
              byte num8 = (byte) ((_parentRotations + (int) prefabByName.RotationsToNorth + (int) num7) % 4);
              this.AddPrefab(new PrefabDataInstance(this.worldBuilder.PrefabManager.PrefabInstanceId++, _position, num8, prefabByName));
              ++this.worldBuilder.WildernessPrefabCount;
              this.SpawnMarkerPartsAndPrefabsWilderness(prefabByName, _parentPosition + marker.Start, (int) num8);
              break;
            }
          }
        }
      }
    }
    GameRandomManager.Instance.FreeGameRandom(gameRandom);
  }

  public int GetNumTownshipNeighbors()
  {
    int townshipNeighbors = 0;
    foreach (StreetTile neighbor in this.GetNeighbors())
    {
      if (neighbor.Township == this.Township)
        ++townshipNeighbors;
    }
    return townshipNeighbors;
  }

  [CompilerGenerated]
  [PublicizedFrom(EAccessModifier.Private)]
  public float \u003CSpawnMarkerPartsAndPrefabs\u003Eb__125_5(Prefab.Marker m)
  {
    return this.Township.rand.RandomFloat;
  }

  [CompilerGenerated]
  [PublicizedFrom(EAccessModifier.Private)]
  public float \u003CSpawnMarkerPartsAndPrefabs\u003Eb__125_7(Prefab.Marker m)
  {
    return this.Township.rand.RandomFloat;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public enum RoadShapeTypes
  {
    straight,
    t,
    intersection,
    cap,
    corner,
  }

  public enum PrefabRotations
  {
    None,
    One,
    Two,
    Three,
  }
}
