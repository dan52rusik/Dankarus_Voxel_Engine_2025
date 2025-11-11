// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.WildernessPlanner
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using UniLinq;
using UnityEngine;

#nullable disable
namespace WorldGenerationEngineFinal;

public class WildernessPlanner
{
  [PublicizedFrom(EAccessModifier.Private)]
  public const int cWildernessSpawnTries = 20;
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly WorldBuilder worldBuilder;
  public readonly List<WorldBuilder.WildernessPathInfo> WildernessPathInfos = new List<WorldBuilder.WildernessPathInfo>();

  public WildernessPlanner(WorldBuilder _worldBuilder) => this.worldBuilder = _worldBuilder;

  public IEnumerator Plan(DynamicProperties thisWorldProperties, int worldSeed)
  {
    yield return (object) null;
    MicroStopwatch ms = new MicroStopwatch(true);
    this.WildernessPathInfos.Clear();
    int retries = 0;
    List<StreetTile> validTiles = new List<StreetTile>(200);
    GameRandom rnd = GameRandomManager.Instance.CreateGameRandom(worldSeed + 409651);
    for (int n = 0; n < 5; ++n)
    {
      int count = this.worldBuilder.GetCount(((BiomeType) n).ToString() + "_wilderness", this.worldBuilder.Wilderness);
      if (count < 0)
        count = this.worldBuilder.GetCount("wilderness", this.worldBuilder.Wilderness);
      int poisLeft = count;
      if (poisLeft < 0)
      {
        poisLeft = 200;
        Log.Warning("No wilderness settings in rwgmixer for this world size, using default count of {0}", (object) poisLeft);
      }
      int poisTotal = poisLeft;
      List<StreetTile> biomeTiles = this.GetUnusedWildernessTiles((BiomeType) n);
      for (; poisLeft > 0; --poisLeft)
      {
        this.GetUnusedWildernessTiles(biomeTiles, validTiles);
        if (validTiles.Count != 0)
        {
          for (int tries = 0; tries < 20; ++tries)
          {
            if (this.worldBuilder.IsMessageElapsed())
              yield return (object) this.worldBuilder.SetMessage(string.Format(Localization.Get("xuiRwgWildernessPOIs"), (object) Mathf.FloorToInt((float) (100.0 * (1.0 - (double) poisLeft / (double) poisTotal)))));
            StreetTile streetTile = validTiles[WildernessPlanner.GetLowBiasedRandom(rnd, validTiles.Count)];
            if (!streetTile.Used && streetTile.SpawnPrefabs())
            {
              streetTile.Used = true;
              break;
            }
            ++retries;
          }
        }
        else
          break;
      }
      biomeTiles = (List<StreetTile>) null;
    }
    GameRandomManager.Instance.FreeGameRandom(rnd);
    Log.Out($"WildernessPlanner Plan {this.worldBuilder.WildernessPrefabCount} prefabs spawned, in {(ValueType) (float) ((double) ms.ElapsedMilliseconds * (1.0 / 1000.0))}, retries {retries}, r={Rand.Instance.PeekSample():x}");
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static int GetLowBiasedRandom(GameRandom rnd, int max)
  {
    double randomFloat = (double) rnd.RandomFloat;
    return (int) (randomFloat * randomFloat * (double) max);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public List<StreetTile> GetUnusedWildernessTiles(BiomeType _biome)
  {
    return ((IEnumerable) this.worldBuilder.StreetTileMap).Cast<StreetTile>().Where<StreetTile>((Func<StreetTile, bool>) ([PublicizedFrom(EAccessModifier.Internal)] (st) => !st.OverlapsRadiation && !st.AllIsWater && (st.District == null || st.District.name == "wilderness") && !st.Used && st.BiomeType == _biome)).ToList<StreetTile>();
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void GetUnusedWildernessTiles(List<StreetTile> _list, List<StreetTile> _resultList)
  {
    IOrderedEnumerable<StreetTile> collection = _list.Cast<StreetTile>().Where<StreetTile>((Func<StreetTile, bool>) ([PublicizedFrom(EAccessModifier.Internal)] (st) => !st.Used)).OrderByDescending<StreetTile, int>((Func<StreetTile, int>) ([PublicizedFrom(EAccessModifier.Private)] (st) => this.distanceFromClosestTownship(st)));
    _resultList.Clear();
    _resultList.AddRange((IEnumerable<StreetTile>) collection);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static bool hasTownshipNeighbor(StreetTile st)
  {
    foreach (StreetTile streetTile in st.GetNeighbors8way())
    {
      if (streetTile.Township != null && !streetTile.Township.IsWilderness())
        return true;
    }
    return false;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public int distanceFromClosestTownship(StreetTile st)
  {
    int num1 = int.MaxValue;
    foreach (Township township in this.worldBuilder.Townships)
    {
      int num2 = Vector2i.DistanceSqrInt(st.WorldPositionCenter, this.worldBuilder.GetStreetTileGrid(township.GridCenter).WorldPositionCenter);
      if (num2 < num1)
        num1 = num2;
    }
    return num1;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static bool hasPrefabNeighbor(StreetTile st)
  {
    foreach (StreetTile streetTile in st.GetNeighbors8way())
    {
      if (streetTile.HasPrefabs)
        return true;
    }
    return false;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static float distanceSqr(Vector2 pointA, Vector2 pointB)
  {
    Vector2 vector2 = pointA - pointB;
    return (float) ((double) vector2.x * (double) vector2.x + (double) vector2.y * (double) vector2.y);
  }

  [CompilerGenerated]
  [PublicizedFrom(EAccessModifier.Private)]
  public int \u003CGetUnusedWildernessTiles\u003Eb__7_1(StreetTile st)
  {
    return this.distanceFromClosestTownship(st);
  }
}
