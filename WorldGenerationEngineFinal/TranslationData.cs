// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.TranslationData
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

#nullable disable
namespace WorldGenerationEngineFinal;

public class TranslationData
{
  public int x;
  public int y;
  public float scale;
  public int rotation;

  public TranslationData(
    int _x,
    int _y,
    float _randomScaleMin = 0.5f,
    float _randomScaleMax = 1.5f,
    int _rotation = -1)
  {
    this.x = _x;
    this.y = _y;
    this.scale = Rand.Instance.Range(_randomScaleMin, _randomScaleMax);
    this.rotation = _rotation;
    if (_rotation >= 0)
      return;
    this.rotation = Rand.Instance.Range(0, 360);
  }

  public TranslationData(int _x, int _y, float _scale, int _rotation)
  {
    this.x = _x;
    this.y = _y;
    this.scale = _scale;
    this.rotation = _rotation;
  }
}
