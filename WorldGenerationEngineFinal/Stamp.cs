// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.Stamp
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using System.Collections.Generic;
using UnityEngine;

#nullable disable
namespace WorldGenerationEngineFinal;

public class Stamp
{
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly WorldBuilder worldBuilder;
  public TranslationData transform;
  public float alpha;
  public bool additive;
  public float scale;
  public bool isCustomColor;
  public Color customColor;
  public Rect Area;
  public float biomeAlphaCutoff;
  public bool isWater;
  public string Name = "";
  public RawStamp stamp;
  [PublicizedFrom(EAccessModifier.Private)]
  public const float oneByoneScale = 1.4f;

  public int imageHeight => this.stamp.height;

  public int imageWidth => this.stamp.width;

  public Stamp(
    WorldBuilder _worldBuilder,
    RawStamp _stamp,
    TranslationData _transData,
    bool _isCustomColor = false,
    Color _customColor = default (Color),
    float _biomeAlphaCutoff = 0.1f,
    bool _isWater = false,
    string stampName = "")
  {
    this.worldBuilder = _worldBuilder;
    this.stamp = _stamp;
    this.transform = _transData;
    this.scale = this.transform.scale;
    this.isCustomColor = _isCustomColor;
    this.customColor = _customColor;
    this.biomeAlphaCutoff = _biomeAlphaCutoff;
    this.isWater = _isWater;
    this.Name = stampName;
    this.alpha = 1f;
    this.additive = false;
    int rotation = this.transform.rotation;
    int num1 = (int) ((double) _stamp.width * (double) this.scale * 1.3999999761581421);
    int num2 = (int) ((double) _stamp.height * (double) this.scale * 1.3999999761581421);
    int x1 = this.transform.x - num1 / 2;
    int x2 = this.transform.x + num1 / 2;
    int y1 = this.transform.y - num2 / 2;
    int y2 = this.transform.y + num2 / 2;
    int x3 = this.transform.x;
    int y3 = this.transform.y;
    Vector2i rotatedPoint1 = this.getRotatedPoint(x1, y1, x3, y3, rotation);
    Vector2i rotatedPoint2 = this.getRotatedPoint(x2, y1, x3, y3, rotation);
    Vector2i rotatedPoint3 = this.getRotatedPoint(x1, y2, x3, y3, rotation);
    Vector2i rotatedPoint4 = this.getRotatedPoint(x2, y2, x3, y3, rotation);
    Vector2 position = new Vector2((float) Mathf.Min(Mathf.Min(rotatedPoint1.x, rotatedPoint2.x), Mathf.Min(rotatedPoint3.x, rotatedPoint4.x)), (float) Mathf.Min(Mathf.Min(rotatedPoint1.y, rotatedPoint2.y), Mathf.Min(rotatedPoint3.y, rotatedPoint4.y)));
    Vector2 vector2 = new Vector2((float) Mathf.Max(Mathf.Max(rotatedPoint1.x, rotatedPoint2.x), Mathf.Max(rotatedPoint3.x, rotatedPoint4.x)), (float) Mathf.Max(Mathf.Max(rotatedPoint1.y, rotatedPoint2.y), Mathf.Max(rotatedPoint3.y, rotatedPoint4.y)));
    this.Area = new Rect(position, vector2 - position);
    if (!this.isWater)
      return;
    if (this.worldBuilder.waterRects == null)
      this.worldBuilder.waterRects = new List<Rect>();
    this.worldBuilder.waterRects.Add(this.Area);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public Color[] rotateColorArray(Color[] src, float angle, int width, int height)
  {
    Color[] colorArray = new Color[width * height];
    double num1 = Math.Sin(Math.PI / 180.0 * (double) angle);
    double num2 = Math.Cos(Math.PI / 180.0 * (double) angle);
    int num3 = width / 2;
    int num4 = height / 2;
    for (int index1 = 0; index1 < height; ++index1)
    {
      for (int index2 = 0; index2 < width; ++index2)
      {
        float num5 = (float) (num2 * (double) (index2 - num3) + num1 * (double) (index1 - num4)) + (float) num3;
        float num6 = (float) (-num1 * (double) (index2 - num3) + num2 * (double) (index1 - num4)) + (float) num4;
        int num7 = (int) num5;
        int num8 = (int) num6;
        float horizontalPerc = num5 - (float) num7;
        float verticalPerc = num6 - (float) num8;
        if (num7 >= 0 && num7 < width && num8 >= 0 && num8 < height)
        {
          Color selfVal = src[num8 * width + num7];
          Color rightVal = selfVal;
          Color upVal = selfVal;
          Color upRightVal = selfVal;
          if (num7 + 1 < width)
            rightVal = src[num8 * width + num7 + 1];
          if (num8 + 1 < height)
            upVal = src[(num8 + 1) * width + num7];
          if (num7 + 1 < width && num8 + 1 < height)
            upRightVal = src[(num8 + 1) * width + num7 + 1];
          colorArray[index1 * width + index2] = this.QuadLerpColor(selfVal, rightVal, upRightVal, upVal, horizontalPerc, verticalPerc);
        }
      }
    }
    return colorArray;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public Color QuadLerpColor(
    Color selfVal,
    Color rightVal,
    Color upRightVal,
    Color upVal,
    float horizontalPerc,
    float verticalPerc)
  {
    return Color.Lerp(Color.Lerp(selfVal, rightVal, horizontalPerc), Color.Lerp(upVal, upRightVal, horizontalPerc), verticalPerc);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public Vector2i getRotatedPoint(int x, int y, int cx, int cy, int angle)
  {
    double num1 = Math.Cos((double) angle);
    double num2 = Math.Sin((double) angle);
    return new Vector2i(Mathf.RoundToInt((float) ((double) (x - cx) * num1 - (double) (y - cy) * num2) + (float) cx), Mathf.RoundToInt((float) ((double) (x - cx) * num2 + (double) (y - cy) * num1) + (float) cy));
  }
}
