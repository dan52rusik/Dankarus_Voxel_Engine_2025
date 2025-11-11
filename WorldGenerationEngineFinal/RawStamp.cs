// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.RawStamp
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

#nullable disable
namespace WorldGenerationEngineFinal;

public class RawStamp
{
  public string name;
  public float heightConst;
  public float[] heightPixels;
  public float alphaConst;
  public float[] alphaPixels;
  public float[] waterPixels;
  public int width;
  public int height;

  public bool hasWater => this.waterPixels != null;

  public void SmoothAlpha(int _boxSize)
  {
    float[] numArray = new float[this.alphaPixels.Length];
    for (int index1 = 0; index1 < this.height; ++index1)
    {
      for (int index2 = 0; index2 < this.width; ++index2)
      {
        double num1 = 0.0;
        int num2 = 0;
        for (int index3 = -1; index3 < _boxSize; ++index3)
        {
          int num3 = index1 + index3;
          if (num3 >= 0 && num3 < this.height)
          {
            for (int index4 = -1; index4 < _boxSize; ++index4)
            {
              int num4 = index2 + index4;
              if (num4 >= 0 && num4 < this.width)
              {
                num1 += (double) this.alphaPixels[num4 + num3 * this.width];
                ++num2;
              }
            }
          }
        }
        double num5 = num1 / (double) num2;
        numArray[index2 + index1 * this.width] = (float) num5;
      }
    }
    this.alphaPixels = numArray;
  }

  public void BoxAlpha()
  {
    for (int index1 = 0; index1 < this.height; index1 += 4)
    {
      for (int index2 = 0; index2 < this.width; index2 += 4)
      {
        int num1 = index2 + index1 * this.width;
        double num2 = 0.0;
        for (int index3 = 0; index3 < 4; ++index3)
        {
          for (int index4 = 0; index4 < 4; ++index4)
            num2 += (double) this.alphaPixels[num1 + index4 + index3 * this.width];
        }
        double num3 = num2 / 16.0;
        for (int index5 = 0; index5 < 4; ++index5)
        {
          for (int index6 = 0; index6 < 4; ++index6)
            this.alphaPixels[num1 + index6 + index5 * this.width] = (float) num3;
        }
      }
    }
  }
}
