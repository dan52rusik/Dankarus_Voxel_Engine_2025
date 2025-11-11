// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.StampGroup
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System.Collections.Generic;

#nullable disable
namespace WorldGenerationEngineFinal;

public class StampGroup
{
  public string Name;
  public List<Stamp> Stamps;

  public StampGroup(string _name)
  {
    this.Name = _name;
    this.Stamps = new List<Stamp>();
  }
}
