// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.StreetTileShared
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System.Collections.Generic;

#nullable disable
namespace WorldGenerationEngineFinal;

public class StreetTileShared
{
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly WorldBuilder worldBuilder;
  public readonly FastTags<TagGroup.Poi> traderTag = FastTags<TagGroup.Poi>.Parse("trader");
  public readonly FastTags<TagGroup.Poi> wildernessTag = FastTags<TagGroup.Poi>.Parse("wilderness");
  public readonly string[] RoadShapes = new string[5]
  {
    "rwg_tile_straight",
    "rwg_tile_t",
    "rwg_tile_intersection",
    "rwg_tile_cap",
    "rwg_tile_corner"
  };
  public readonly string[] RoadShapesDistrict = new string[5]
  {
    "rwg_tile_{0}straight",
    "rwg_tile_{0}t",
    "rwg_tile_{0}intersection",
    "rwg_tile_{0}cap",
    "rwg_tile_{0}corner"
  };
  [PublicizedFrom(EAccessModifier.Private)]
  public bool[][] RoadShapeExits = new bool[5][]
  {
    new bool[4]{ true, false, true, false },
    new bool[4]{ true, true, true, false },
    new bool[4]{ true, true, true, true },
    new bool[4]{ false, false, true, false },
    new bool[4]{ false, true, true, false }
  };
  public readonly List<int> RoadShapeExitCounts = new List<int>()
  {
    2,
    3,
    4,
    1,
    2
  };
  public readonly List<bool[][]> RoadShapeExitsPerRotation = new List<bool[][]>();
  public readonly Vector2i[] dir4way = new Vector2i[4]
  {
    new Vector2i(0, 1),
    new Vector2i(1, 0),
    new Vector2i(0, -1),
    new Vector2i(-1, 0)
  };
  public readonly Vector2i[] dir8way = new Vector2i[8]
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
  public readonly Vector2i[] dir9way = new Vector2i[9]
  {
    new Vector2i(0, 1),
    new Vector2i(1, 1),
    new Vector2i(1, 0),
    new Vector2i(1, -1),
    new Vector2i(0, 0),
    new Vector2i(0, -1),
    new Vector2i(-1, -1),
    new Vector2i(-1, 0),
    new Vector2i(-1, 1)
  };

  public StreetTileShared(WorldBuilder _worldBuilder)
  {
    this.worldBuilder = _worldBuilder;
    for (int index1 = 0; index1 < this.RoadShapeExits.Length; ++index1)
    {
      bool[][] flagArray1 = new bool[4][];
      for (int index2 = 0; index2 < 4; ++index2)
      {
        bool[][] flagArray2 = flagArray1;
        int index3 = index2;
        bool[] flagArray3;
        switch (index2 - 1)
        {
          case 0:
            flagArray3 = new bool[4]
            {
              this.RoadShapeExits[index1][3],
              this.RoadShapeExits[index1][0],
              this.RoadShapeExits[index1][1],
              this.RoadShapeExits[index1][2]
            };
            break;
          case 1:
            flagArray3 = new bool[4]
            {
              this.RoadShapeExits[index1][2],
              this.RoadShapeExits[index1][3],
              this.RoadShapeExits[index1][0],
              this.RoadShapeExits[index1][1]
            };
            break;
          case 2:
            flagArray3 = new bool[4]
            {
              this.RoadShapeExits[index1][1],
              this.RoadShapeExits[index1][2],
              this.RoadShapeExits[index1][3],
              this.RoadShapeExits[index1][0]
            };
            break;
          default:
            flagArray3 = new bool[4]
            {
              this.RoadShapeExits[index1][0],
              this.RoadShapeExits[index1][1],
              this.RoadShapeExits[index1][2],
              this.RoadShapeExits[index1][3]
            };
            break;
        }
        flagArray2[index3] = flagArray3;
      }
      this.RoadShapeExitsPerRotation.Add(flagArray1);
    }
  }
}
