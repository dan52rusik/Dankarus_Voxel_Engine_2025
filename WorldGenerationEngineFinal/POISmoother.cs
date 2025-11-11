// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.POISmoother
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System.Collections;
using UnityEngine;

#nullable disable
namespace WorldGenerationEngineFinal;

public class POISmoother
{
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly WorldBuilder worldBuilder;

  public POISmoother(WorldBuilder _worldBuilder) => this.worldBuilder = _worldBuilder;

  public IEnumerator SmoothStreetTiles()
  {
    yield return (object) null;
    MicroStopwatch ms = new MicroStopwatch(true);
    float width = (float) this.worldBuilder.StreetTileMap.GetLength(0);
    float height = (float) this.worldBuilder.StreetTileMap.GetLength(1);
    float current = 0.0f;
    float total = width * height;
    for (int x = 0; (double) x < (double) width; ++x)
    {
      for (int index = 0; (double) index < (double) height; ++index)
      {
        ++current;
        this.worldBuilder.StreetTileMap[x, index].SmoothTownshipTerrain();
        this.worldBuilder.StreetTileMap[x, index].UpdateValidity();
      }
      if (this.worldBuilder.IsMessageElapsed())
        yield return (object) this.worldBuilder.SetMessage(string.Format(Localization.Get("xuiRwgSmoothingStreetTiles"), (object) Mathf.RoundToInt((float) ((double) current / (double) total * 100.0))));
    }
    Log.Out("POISmoother SmoothStreetTiles in {0}", (object) (float) ((double) ms.ElapsedMilliseconds * (1.0 / 1000.0)));
  }
}
