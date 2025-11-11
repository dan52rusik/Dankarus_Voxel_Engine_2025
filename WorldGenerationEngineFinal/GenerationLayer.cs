// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.GenerationLayer
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System.Collections.Generic;

#nullable disable
namespace WorldGenerationEngineFinal;

public class GenerationLayer
{
  public int x;
  public int y;
  public int Range;
  public List<TranslationData> children;

  public GenerationLayer(int _x, int _y, int _range)
  {
    this.x = _x;
    this.y = _y;
    this.Range = _range;
    this.children = new List<TranslationData>();
  }
}
