// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.RandomCountyNameGenerator
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using System.Collections.Generic;

#nullable disable
namespace WorldGenerationEngineFinal;

public static class RandomCountyNameGenerator
{
  [PublicizedFrom(EAccessModifier.Private)]
  public static string constenants = "b,c,d,f,g,h,j,k,l,m,n,p,r,s,t,v,w,x,y,z";
  [PublicizedFrom(EAccessModifier.Private)]
  public static string vowels = "a,e,i,o,u";
  [PublicizedFrom(EAccessModifier.Private)]
  public static string[] prefixes = new string[12]
  {
    "Old",
    "",
    "New",
    "",
    "North",
    "",
    "East",
    "",
    "South",
    "",
    "West",
    ""
  };
  [PublicizedFrom(EAccessModifier.Private)]
  public static string[] suffixes = new string[4]
  {
    " County",
    " Territory",
    " Valley",
    " Mountains"
  };

  public static string GetName(int _seed)
  {
    string[] strArray1 = RandomCountyNameGenerator.constenants.Split(',', StringSplitOptions.None);
    string[] strArray2 = RandomCountyNameGenerator.vowels.Split(',', StringSplitOptions.None);
    List<string> stringList = new List<string>();
    for (int index = 0; index < strArray2.Length; ++index)
      stringList.Add(strArray2[index]);
    for (int index1 = 0; index1 < strArray1.Length; ++index1)
    {
      for (int index2 = 0; index2 < strArray2.Length; ++index2)
        stringList.Add(strArray1[index1] + strArray2[index2]);
    }
    string[] array = stringList.ToArray();
    Rand.Instance.SetSeed(_seed);
    string str1 = "";
    string str2 = "";
    string prefix = RandomCountyNameGenerator.prefixes[Rand.Instance.Range(0, RandomCountyNameGenerator.prefixes.Length)];
    if (prefix.Length > 0)
      str2 = $"{str2}{prefix} ";
    int num = Rand.Instance.Range(3, 5);
    for (int index = 0; index < num; ++index)
      str1 += array[Rand.Instance.Range(0, array.Length)];
    string str3 = str1.Substring(0, 1);
    string str4 = str1.Remove(0, 1);
    string str5 = str3.ToUpper() + str4;
    string name = str2 + str5;
    string suffix = RandomCountyNameGenerator.suffixes[Rand.Instance.Range(0, RandomCountyNameGenerator.suffixes.Length)];
    if (suffix.Length > 0)
      name += suffix;
    return name;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  static RandomCountyNameGenerator()
  {
  }
}
