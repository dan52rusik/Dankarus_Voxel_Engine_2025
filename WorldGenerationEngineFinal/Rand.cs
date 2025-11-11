// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.Rand
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using UnityEngine;

#nullable disable
namespace WorldGenerationEngineFinal;

public class Rand
{
  [PublicizedFrom(EAccessModifier.Private)]
  public static Rand instance;
  public GameRandom gameRandom;

  public static Rand Instance
  {
    get
    {
      if (Rand.instance == null)
        Rand.instance = new Rand();
      return Rand.instance;
    }
  }

  public Rand() => this.gameRandom = GameRandomManager.Instance.CreateGameRandom();

  public Rand(int seed)
  {
    this.gameRandom = GameRandomManager.Instance.CreateGameRandom();
    this.SetSeed(seed);
  }

  public void Cleanup()
  {
    GameRandomManager.Instance.FreeGameRandom(this.gameRandom);
    Rand.instance = (Rand) null;
  }

  public void Free() => GameRandomManager.Instance.FreeGameRandom(this.gameRandom);

  public void SetSeed(int seed) => this.gameRandom.SetSeed(seed);

  public float Float() => this.gameRandom.RandomFloat;

  public int Range(int min, int max) => this.gameRandom.RandomRange(min, max);

  public int Range(int max) => this.gameRandom.RandomRange(max);

  public float Range(float min, float max) => this.gameRandom.RandomRange(min, max);

  public int Angle() => this.gameRandom.RandomRange(360);

  public Vector2 RandomOnUnitCircle() => this.gameRandom.RandomOnUnitCircle;

  public int PeekSample() => this.gameRandom.PeekSample();
}
