// Decompiled with JetBrains decompiler
// Type: WaterValue
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System.IO;
using System.Runtime.CompilerServices;

#nullable disable
public struct WaterValue
{
  public const float cTopPerCap = 0.6f;
  public const int MAX_MASS_VALUE = 65535 /*0xFFFF*/;
  public static readonly WaterValue Empty = new WaterValue(0);
  public static readonly WaterValue Full = new WaterValue(19500);
  [PublicizedFrom(EAccessModifier.Private)]
  public ushort mass;

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public bool HasMass() => this.mass > (ushort) 195;

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public int GetMass() => (int) this.mass;

  public float GetMassPercent()
  {
    if (this.mass <= (ushort) 195)
      return 0.0f;
    return this.mass >= (ushort) 15600 ? 1f : (float) ((int) this.mass - 195) / 15405f;
  }

  public void SetMass(int value)
  {
    this.mass = (ushort) Utils.FastClamp(value, 0, (int) ushort.MaxValue);
  }

  public override string ToString() => $"Raw Mass: {this.mass:d}";

  public long RawData => (long) this.mass;

  public static WaterValue FromRawData(long rawData)
  {
    return new WaterValue() { mass = (ushort) rawData };
  }

  public static WaterValue FromBlockType(int type)
  {
    return type == 240 /*0xF0*/ || type == 241 || type == 242 ? new WaterValue(19500) : WaterValue.Empty;
  }

  public static WaterValue FromStream(BinaryReader _reader)
  {
    WaterValue waterValue = new WaterValue();
    waterValue.Read(_reader);
    return waterValue;
  }

  public WaterValue(BlockValue _bv) => this.mass = _bv.isWater ? (ushort) 19500 : (ushort) 0;

  public WaterValue(int mass)
  {
    this.mass = (ushort) Utils.FastClamp(mass, 0, (int) ushort.MaxValue);
  }

  public void Write(BinaryWriter writer) => writer.Write(this.mass);

  public void Read(BinaryReader reader) => this.mass = reader.ReadUInt16();

  public static int SerializedLength() => 2;

  [PublicizedFrom(EAccessModifier.Private)]
  static WaterValue()
  {
  }
}
