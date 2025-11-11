// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.StampManager
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;

#nullable disable
namespace WorldGenerationEngineFinal;

public class StampManager
{
  [PublicizedFrom(EAccessModifier.Private)]
  public const float cAlphaCutoff = 1E-05f;
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly WorldBuilder worldBuilder;
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly Dictionary<string, RawStamp> AllStamps = new Dictionary<string, RawStamp>();
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly List<RawStamp> tempGetStampList = new List<RawStamp>();

  public StampManager(WorldBuilder _worldBuilder) => this.worldBuilder = _worldBuilder;

  public void ClearStamps() => this.AllStamps.Clear();

  public void DrawStampGroup(StampGroup _group, float[] _dest, float[] _waterDest, int size)
  {
    MicroStopwatch microStopwatch = new MicroStopwatch(true);
    for (int index = 0; index < _group.Stamps.Count; ++index)
    {
      Stamp stamp = _group.Stamps[index];
      if (stamp != null)
      {
        this.DrawStamp(_dest, _waterDest, stamp);
        if (this.worldBuilder.IsCanceled)
          break;
      }
    }
    Log.Out("DrawStampGroup '{0}', count {1}, in {2}", (object) _group.Name, (object) _group.Stamps.Count, (object) (float) ((double) microStopwatch.ElapsedMilliseconds * (1.0 / 1000.0)));
  }

  public void DrawStampGroup(StampGroup _group, Color32[] _image, int size, float _stampScale = 1f)
  {
    MicroStopwatch microStopwatch = new MicroStopwatch(true);
    for (int index = 0; index < _group.Stamps.Count; ++index)
    {
      Stamp stamp = _group.Stamps[index];
      if (stamp != null)
      {
        int _x = (int) ((double) stamp.transform.x * (double) _stampScale);
        int _y = (int) ((double) stamp.transform.y * (double) _stampScale);
        StampManager.DrawStamp(_image, stamp.stamp, _x, _y, size, size, stamp.imageWidth, stamp.imageHeight, stamp.alpha, stamp.scale * _stampScale, stamp.isCustomColor, (Color32) stamp.customColor, stamp.biomeAlphaCutoff, (float) stamp.transform.rotation, stamp.isWater);
        if (this.worldBuilder.IsCanceled)
          break;
      }
    }
    Log.Out("DrawStampGroup c32 '{0}', count {1}, in {2}", (object) _group.Name, (object) _group.Stamps.Count, (object) (float) ((double) microStopwatch.ElapsedMilliseconds * (1.0 / 1000.0)));
  }

  public void DrawWaterStampGroup(StampGroup _group, float[] _dest, int _destSize)
  {
    MicroStopwatch microStopwatch = new MicroStopwatch(true);
    for (int index = 0; index < _group.Stamps.Count; ++index)
    {
      Stamp stamp = _group.Stamps[index];
      if (stamp != null)
      {
        StampManager.DrawWaterStamp(stamp, _dest, _destSize);
        if (this.worldBuilder.IsCanceled)
          break;
      }
    }
    Log.Out("DrawWaterStampGroup '{0}', count {1}, in {2}", (object) _group.Name, (object) _group.Stamps.Count, (object) (float) ((double) microStopwatch.ElapsedMilliseconds * (1.0 / 1000.0)));
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static float CalcRotatedValue(
    float x1,
    float y1,
    float[] src,
    double sine,
    double cosine,
    int width,
    int height,
    bool isWater = false)
  {
    int num1 = width >> 1;
    int num2 = height >> 1;
    float num3 = (float) (cosine * ((double) x1 - (double) num1) + sine * ((double) y1 - (double) num2)) + (float) num1;
    int num4 = (int) num3;
    if ((long) (uint) num4 >= (long) width)
      return 0.0f;
    float num5 = (float) (-sine * ((double) x1 - (double) num1) + cosine * ((double) y1 - (double) num2)) + (float) num2;
    int num6 = (int) num5;
    if ((long) (uint) num6 >= (long) height)
      return 0.0f;
    int index = num4 + num6 * width;
    float num7 = src[index];
    float num8 = num4 + 1 >= width ? num7 : src[index + 1];
    float num9 = num6 + 1 >= height ? num7 : src[index + width];
    float num10 = num4 + 1 >= width || num6 + 1 >= height ? num7 : src[index + width + 1];
    if (isWater && ((double) num7 > 0.0 || (double) num9 > 0.0 || (double) num8 > 0.0 || (double) num10 > 0.0))
      return num7;
    float num11 = num3 - (float) num4;
    float num12 = num5 - (float) num6;
    float num13 = num7 + (num8 - num7) * num11;
    float num14 = num9 + (num10 - num9) * num11;
    return num13 + (num14 - num13) * num12;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static double CalcRotatedValue(
    StampManager.RotateParams _p,
    double x1,
    double y1,
    float[] src)
  {
    double num1 = _p.cosine * (x1 - _p.halfWidth) + _p.sine * (y1 - _p.halfHeight) + _p.halfWidth;
    int num2 = (int) num1;
    if ((long) (uint) num2 >= (long) _p.width)
      return 0.0;
    double num3 = -_p.sine * (x1 - _p.halfWidth) + _p.cosine * (y1 - _p.halfHeight) + _p.halfHeight;
    int num4 = (int) num3;
    if ((long) (uint) num4 >= (long) _p.height)
      return 0.0;
    int index = num2 + num4 * _p.width;
    double num5 = (double) src[index];
    double num6 = num5;
    if (num2 + 1 < _p.width)
      num6 = (double) src[index + 1];
    double num7 = num5;
    if (num4 + 1 < _p.height)
      num7 = (double) src[index + _p.width];
    double num8 = num5;
    if (num2 + 1 < _p.width && num4 + 1 < _p.height)
      num8 = (double) src[index + _p.width + 1];
    if (_p.isWater && (num5 > 0.0 || num7 > 0.0 || num6 > 0.0 || num8 > 0.0))
      return num5;
    double num9 = num1 - (double) num2;
    double num10 = num3 - (double) num4;
    double num11 = num5 + (num6 - num5) * num9;
    double num12 = num7 + (num8 - num7) * num9;
    return num11 + (num12 - num11) * num10;
  }

  public void DrawStamp(float[] _dest, float[] _waterDest, Stamp stamp)
  {
    int x = stamp.transform.x;
    int y = stamp.transform.y;
    int worldSize = this.worldBuilder.WorldSize;
    float rotation = (float) stamp.transform.rotation;
    StampManager.DrawStamp(_dest, _waterDest, stamp.stamp, x, y, worldSize, worldSize, (double) stamp.alpha, stamp.additive, (double) stamp.scale, stamp.isCustomColor, stamp.customColor, (double) stamp.biomeAlphaCutoff, (double) rotation);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static void DrawStamp(
    float[] _dest,
    float[] _waterDest,
    RawStamp _src,
    int _x,
    int _y,
    int _destWidth,
    int _destHeight,
    double _alpha,
    bool _additive,
    double _scale,
    bool _isCustomColor,
    Color _customColor,
    double _biomeCutoff,
    double _angle)
  {
    _x -= (int) ((double) _src.width * _scale) / 2;
    _y -= (int) ((double) _src.height * _scale) / 2;
    double num1 = Math.PI / 180.0 * _angle;
    double num2 = Math.Sin(num1);
    double num3 = Math.Cos(num1);
    int num4 = (int) Math.Floor((double) (((int) Math.Sqrt((double) (_src.width * _src.width + _src.height * _src.height)) - _src.width) / 2) * _scale);
    int num5 = (int) Math.Floor((double) _src.width * _scale + (double) num4);
    int num6 = -num4;
    int num7 = num6;
    int num8 = _x + num6;
    if (num8 < 0)
      num7 -= num8;
    int num9 = num5;
    int num10 = _x + num5;
    if (num10 >= _destWidth)
      num9 -= num10 - _destWidth;
    int num11 = num6;
    int num12 = _y + num6;
    if (num12 < 0)
      num11 -= num12;
    int num13 = num5;
    int num14 = _y + num5;
    if (num14 >= _destHeight)
      num13 -= num14 - _destHeight;
    StampManager.RotateParams _p = new StampManager.RotateParams();
    _p.sine = num2;
    _p.cosine = num3;
    _p.width = _src.width;
    _p.height = _src.height;
    _p.halfWidth = (double) (_src.width / 2);
    _p.halfHeight = (double) (_src.height / 2);
    for (int index1 = num11; index1 < num13; ++index1)
    {
      int num15 = (_y + index1) * _destWidth;
      double y1 = (double) index1 / _scale;
      for (int index2 = num7; index2 < num9; ++index2)
      {
        double x1 = (double) index2 / _scale;
        double num16 = (double) _src.alphaConst;
        if (_src.alphaPixels != null)
          num16 = StampManager.CalcRotatedValue(_p, x1, y1, _src.alphaPixels);
        if (num16 >= 9.9999997473787516E-06)
        {
          int index3 = _x + index2 + num15;
          if (_isCustomColor)
          {
            if (num16 > _biomeCutoff)
            {
              _dest[index3] = _customColor.r;
              _waterDest[index3] = _customColor.b;
            }
          }
          else
          {
            double num17 = (double) _src.heightConst;
            if (_src.heightPixels != null)
              num17 = StampManager.CalcRotatedValue(_p, x1, y1, _src.heightPixels);
            double num18 = num17;
            if (_src.waterPixels != null)
              num18 = StampManager.CalcRotatedValue(_p, x1, y1, _src.waterPixels);
            double num19 = num16 * _alpha;
            double num20 = (double) _dest[index3];
            double num21 = !_additive ? num20 + (num17 - num20) * num19 : num20 + num17 * num19;
            _dest[index3] = (float) num21;
            double num22 = (double) _waterDest[index3];
            double num23 = num22 + (num18 - num22) * num19;
            _waterDest[index3] = (float) num23;
          }
        }
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void DrawStamp(Color32[] _dest, Stamp stamp)
  {
    int x = stamp.transform.x;
    int y = stamp.transform.y;
    int worldSize = this.worldBuilder.WorldSize;
    float rotation = (float) stamp.transform.rotation;
    StampManager.DrawStamp(_dest, stamp.stamp, x, y, worldSize, worldSize, stamp.imageWidth, stamp.imageHeight, stamp.alpha, stamp.scale, stamp.isCustomColor, (Color32) stamp.customColor, stamp.biomeAlphaCutoff, rotation, stamp.isWater);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static void DrawStamp(
    Color32[] _dest,
    RawStamp _src,
    int _x,
    int _y,
    int _destWidth,
    int _destHeight,
    int _srcWidth,
    int _srcHeight,
    float _alpha,
    float _scale,
    bool _isCustomColor,
    Color32 _customColor,
    float _biomeCutoff = 0.1f,
    float _angle = 0.0f,
    bool isWater = false)
  {
    _x -= (int) ((double) _srcWidth * (double) _scale) / 2;
    _y -= (int) ((double) _srcHeight * (double) _scale) / 2;
    double num1 = Math.PI / 180.0 * (double) _angle;
    double sine = Math.Sin(num1);
    double cosine = Math.Cos(num1);
    int num2 = Mathf.FloorToInt((float) (((int) Mathf.Sqrt((float) (_srcWidth * _srcWidth + _srcHeight * _srcHeight)) - _srcWidth) / 2) * _scale);
    int num3 = Mathf.FloorToInt((float) _srcWidth * _scale + (float) num2);
    int num4 = -num2;
    int num5 = num4;
    int num6 = _x + num4;
    if (num6 < 0)
      num5 -= num6;
    int num7 = num3;
    int num8 = _x + num3;
    if (num8 >= _destWidth)
      num7 -= num8 - _destWidth;
    int num9 = num4;
    int num10 = _y + num4;
    if (num10 < 0)
      num9 -= num10;
    int num11 = num3;
    int num12 = _y + num3;
    if (num12 >= _destHeight)
      num11 -= num12 - _destHeight;
    for (int index1 = num9; index1 < num11; ++index1)
    {
      int num13 = (_y + index1) * _destWidth;
      float y1 = (float) index1 / _scale;
      for (int index2 = num5; index2 < num7; ++index2)
      {
        float num14 = _src.alphaConst;
        if (_src.alphaPixels != null)
          num14 = StampManager.CalcRotatedValue((float) index2 / _scale, y1, _src.alphaPixels, sine, cosine, _srcWidth, _srcHeight, isWater);
        if ((double) num14 > (double) _biomeCutoff)
        {
          int index3 = _x + index2 + num13;
          _dest[index3] = _customColor;
        }
      }
    }
  }

  public static void DrawWaterStamp(Stamp stamp, float[] _dest, int _destSize)
  {
    if (!stamp.isWater)
      throw new ArgumentException("DrawWaterStamp called with non-water stamp " + stamp.Name);
    float[] _src = stamp.stamp.waterPixels;
    if (_src == null)
    {
      _src = stamp.stamp.alphaPixels;
      if (_src == null)
        return;
    }
    int x = stamp.transform.x;
    int y = stamp.transform.y;
    float rotation = (float) stamp.transform.rotation;
    StampManager.DrawWaterStamp(_dest, _src, x, y, _destSize, _destSize, stamp.imageWidth, stamp.imageHeight, stamp.alpha, (double) stamp.scale, stamp.isCustomColor, stamp.customColor, rotation);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static void DrawWaterStamp(
    float[] _dest,
    float[] _src,
    int _x,
    int _y,
    int _destWidth,
    int _destHeight,
    int _srcWidth,
    int _srcHeight,
    float _alpha,
    double _scale,
    bool _isCustomColor,
    Color _customColor,
    float _angle)
  {
    _x -= (int) ((double) _srcWidth * _scale) / 2;
    _y -= (int) ((double) _srcHeight * _scale) / 2;
    double num1 = Math.PI / 180.0 * (double) _angle;
    double num2 = Math.Sin(num1);
    double num3 = Math.Cos(num1);
    int num4 = (int) Math.Floor((double) (((int) Math.Sqrt((double) (_srcWidth * _srcWidth + _srcHeight * _srcHeight)) - _srcWidth) / 2) * _scale);
    int num5 = (int) Math.Floor((double) _srcWidth * _scale + (double) num4);
    int num6 = -num4;
    int num7 = num6;
    int num8 = _x + num6;
    if (num8 < 0)
      num7 -= num8;
    int num9 = num5;
    int num10 = _x + num5;
    if (num10 >= _destWidth)
      num9 -= num10 - _destWidth;
    int num11 = num6;
    int num12 = _y + num6;
    if (num12 < 0)
      num11 -= num12;
    int num13 = num5;
    int num14 = _y + num5;
    if (num14 >= _destHeight)
      num13 -= num14 - _destHeight;
    StampManager.RotateParams _p = new StampManager.RotateParams();
    _p.sine = num2;
    _p.cosine = num3;
    _p.width = _srcWidth;
    _p.height = _srcHeight;
    _p.halfWidth = (double) (_srcWidth / 2);
    _p.halfHeight = (double) (_srcHeight / 2);
    _p.isWater = true;
    for (int index1 = num11; index1 < num13; ++index1)
    {
      int num15 = (_y + index1) * _destWidth;
      double y1 = (double) index1 / _scale;
      for (int index2 = num7; index2 < num9; ++index2)
      {
        int index3 = _x + index2 + num15;
        if ((double) _dest[index3] <= 0.0)
        {
          float num16 = (float) StampManager.CalcRotatedValue(_p, (double) index2 / _scale, y1, _src);
          if ((double) num16 >= 9.9999997473787516E-06)
            _dest[index3] = !_isCustomColor ? num16 : _customColor.b;
        }
      }
    }
  }

  public static void DrawBiomeStamp(
    Color32[] _dest,
    float[] _src,
    int _x,
    int _y,
    int _destWidth,
    int _destHeight,
    int _srcWidth,
    int _srcHeight,
    float _scale,
    Color32 _destColor,
    float _biomeCutoff = 0.1f,
    float _angle = 0.0f)
  {
    _x -= (int) ((double) _srcWidth * (double) _scale) / 2;
    _y -= (int) ((double) _srcHeight * (double) _scale) / 2;
    double num1 = Math.PI / 180.0 * (double) _angle;
    double sine = Math.Sin(num1);
    double cosine = Math.Cos(num1);
    int num2 = Mathf.FloorToInt((float) (((int) Mathf.Sqrt((float) (_srcWidth * _srcWidth + _srcHeight * _srcHeight)) - _srcWidth) / 2) * _scale);
    int num3 = Mathf.FloorToInt((float) _srcWidth * _scale + (float) num2);
    int num4 = -num2;
    int num5 = num4;
    int num6 = _x + num4;
    if (num6 < 0)
      num5 -= num6;
    int num7 = num3;
    int num8 = _x + num3;
    if (num8 >= _destWidth)
      num7 -= num8 - _destWidth;
    int num9 = num4;
    int num10 = _y + num4;
    if (num10 < 0)
      num9 -= num10;
    int num11 = num3;
    int num12 = _y + num3;
    if (num12 >= _destHeight)
      num11 -= num12 - _destHeight;
    for (int index1 = num9; index1 < num11; ++index1)
    {
      int num13 = (_y + index1) * _destWidth;
      float y1 = (float) index1 / _scale;
      for (int index2 = num5; index2 < num7; ++index2)
      {
        int index3 = _x + index2 + num13;
        if ((double) StampManager.CalcRotatedValue((float) index2 / _scale, y1, _src, sine, cosine, _srcWidth, _srcHeight) > (double) _biomeCutoff)
          _dest[index3] = _destColor;
      }
    }
  }

  public bool TryGetStamp(string terrainTypeName, string comboTypeName, out RawStamp tmp)
  {
    return this.TryGetStamp(comboTypeName, out tmp) || this.TryGetStamp(terrainTypeName, out tmp);
  }

  public bool TryGetStamp(string _baseName, out RawStamp _output)
  {
    return this.TryGetStamp(_baseName, out _output, Rand.Instance);
  }

  public bool TryGetStamp(string _baseName, out RawStamp _output, Rand _rand)
  {
    lock (this.tempGetStampList)
    {
      foreach (KeyValuePair<string, RawStamp> allStamp in this.AllStamps)
      {
        if (allStamp.Key.StartsWith(_baseName))
          this.tempGetStampList.Add(allStamp.Value);
      }
      if (this.tempGetStampList.Count == 0)
      {
        _output = (RawStamp) null;
        return false;
      }
      _output = this.tempGetStampList[_rand.Range(0, this.tempGetStampList.Count)];
      this.tempGetStampList.Clear();
      return true;
    }
  }

  public RawStamp GetStamp(string _baseName, Rand _rand = null)
  {
    if (_rand == null)
      _rand = Rand.Instance;
    RawStamp _output;
    return !this.TryGetStamp(_baseName, out _output, _rand) ? (RawStamp) null : _output;
  }

  public IEnumerator LoadStamps()
  {
    if (this.AllStamps.Count <= 0)
    {
      MicroStopwatch ms = new MicroStopwatch(true);
      this.AllStamps.Clear();
      List<PathAbstractions.AbstractedLocation> stampMaps = PathAbstractions.RwgStampsSearchPaths.GetAvailablePathsList(_ignoreDuplicateNames: true);
      MicroStopwatch msRestart = new MicroStopwatch(true);
      int i;
      for (i = 0; i < stampMaps.Count; ++i)
      {
        string extension = stampMaps[i].Extension;
        if (!(extension != ".exr") || !(extension != ".raw"))
        {
          this.LoadStamp(stampMaps[i]);
          if (msRestart.ElapsedMilliseconds > 500L)
          {
            yield return (object) null;
            msRestart.ResetAndRestart();
          }
        }
      }
      for (i = 0; i < stampMaps.Count; ++i)
      {
        this.LoadStamp(stampMaps[i]);
        if (msRestart.ElapsedMilliseconds > 500L)
        {
          yield return (object) null;
          msRestart.ResetAndRestart();
        }
      }
      Log.Out("LoadStamps in {0}", (object) (float) ((double) ms.ElapsedMilliseconds * (1.0 / 1000.0)));
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void LoadStamp(PathAbstractions.AbstractedLocation _path)
  {
    string fileNameNoExtension = _path.FileNameNoExtension;
    if (this.AllStamps.ContainsKey(fileNameNoExtension))
      return;
    MicroStopwatch microStopwatch = new MicroStopwatch(true);
    Color[] colorArray;
    if (_path.Extension == ".raw")
    {
      colorArray = Utils.LoadRawStampFileArray(_path.FullPath);
    }
    else
    {
      if (_path.Extension == ".exr")
        return;
      Texture2D texture2D = TextureUtils.LoadTexture(_path.FullPath);
      colorArray = texture2D.GetPixels();
      if (Application.isPlaying)
        UnityEngine.Object.Destroy((UnityEngine.Object) texture2D);
    }
    if (colorArray == null)
    {
      Log.Error("LoadStamp {0} failed", (object) _path.FullPath);
    }
    else
    {
      float[] numArray1 = (float[]) null;
      float r = colorArray[0].r;
      for (int index1 = 1; index1 < colorArray.Length; ++index1)
      {
        if ((double) r != (double) colorArray[index1].r)
        {
          numArray1 = new float[colorArray.Length];
          for (int index2 = 0; index2 < colorArray.Length; ++index2)
            numArray1[index2] = colorArray[index2].r;
          break;
        }
      }
      float[] numArray2 = (float[]) null;
      float a = colorArray[0].a;
      for (int index3 = 1; index3 < colorArray.Length; ++index3)
      {
        if ((double) a != (double) colorArray[index3].a)
        {
          numArray2 = new float[colorArray.Length];
          for (int index4 = 0; index4 < colorArray.Length; ++index4)
            numArray2[index4] = colorArray[index4].a;
          break;
        }
      }
      float[] numArray3 = (float[]) null;
      if (!fileNameNoExtension.Contains("rwg_tile"))
      {
        for (int index = 0; index < colorArray.Length; ++index)
        {
          Color color = colorArray[index];
          if ((double) color.b != 0.0)
          {
            if (numArray3 == null)
              numArray3 = new float[colorArray.Length];
            numArray3[index] = color.b;
          }
        }
      }
      int num = (int) Mathf.Sqrt((float) colorArray.Length);
      RawStamp rawStamp = new RawStamp()
      {
        name = fileNameNoExtension,
        heightConst = r,
        heightPixels = numArray1,
        alphaConst = a,
        alphaPixels = numArray2,
        waterPixels = numArray3,
        height = num,
        width = num
      };
      this.AllStamps[fileNameNoExtension] = rawStamp;
      if (fileNameNoExtension.StartsWith("mountains_"))
        rawStamp.SmoothAlpha(5);
      if (fileNameNoExtension.StartsWith("desert_mountains_"))
        rawStamp.SmoothAlpha(3);
      Log.Out("LoadStamp {0}, size {1}, height {2} ({3}), alpha {4} ({5}), water {6}, in {7}", (object) _path.FullPath, (object) num, (object) (numArray1 != null), (object) r, (object) (numArray2 != null), (object) a, (object) (numArray3 != null), (object) (float) ((double) microStopwatch.ElapsedMilliseconds * (1.0 / 1000.0)));
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static void SaveEXR(string cleanname, Color[] colors, bool _replace = true)
  {
    string path = $"{GameIO.GetGameDir("Data/Stamps")}/{cleanname}.exr";
    if (!_replace && File.Exists(path))
      return;
    int num = (int) Mathf.Sqrt((float) colors.Length);
    Texture2D tex = new Texture2D(num, num, TextureFormat.RGBAFloat, false);
    tex.SetPixels(colors);
    File.WriteAllBytes(path, tex.EncodeToEXR(Texture2D.EXRFlags.None));
    Log.Out("SaveEXR {0}", (object) path);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public class RotateParams
  {
    public double sine;
    public double cosine;
    public int width;
    public int height;
    public double halfWidth;
    public double halfHeight;
    public bool isWater;
  }
}
