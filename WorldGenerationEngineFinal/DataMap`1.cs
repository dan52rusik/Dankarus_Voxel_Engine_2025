// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.DataMap`1
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

#nullable disable
namespace WorldGenerationEngineFinal;

public class DataMap<T>
{
  public T[,] data;

  public DataMap(int tileWidth, T defaultValue)
  {
    this.data = new T[tileWidth, tileWidth];
    for (int index1 = 0; index1 < this.data.GetLength(0); ++index1)
    {
      for (int index2 = 0; index2 < this.data.GetLength(1); ++index2)
        this.data[index1, index2] = defaultValue;
    }
  }
}
