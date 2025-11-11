// Decompiled with JetBrains decompiler
// Type: WaterStats
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using Unity.Collections;

#nullable disable
public struct WaterStats
{
  public int NumChunksProcessed;
  public int NumChunksActive;
  public int NumFlowEvents;
  public int NumVoxelsProcessed;
  public int NumVoxelsPutToSleep;
  public int NumVoxelsWokeUp;

  public static WaterStats Sum(NativeArray<WaterStats> array)
  {
    WaterStats waterStats = new WaterStats();
    for (int index = 0; index < array.Length; ++index)
      waterStats += array[index];
    return waterStats;
  }

  public static WaterStats operator +(WaterStats a, WaterStats b)
  {
    return new WaterStats()
    {
      NumChunksProcessed = a.NumChunksProcessed + b.NumChunksProcessed,
      NumChunksActive = a.NumChunksActive + b.NumChunksActive,
      NumFlowEvents = a.NumFlowEvents + b.NumFlowEvents,
      NumVoxelsProcessed = a.NumVoxelsProcessed + b.NumVoxelsProcessed,
      NumVoxelsPutToSleep = a.NumVoxelsPutToSleep + b.NumVoxelsPutToSleep,
      NumVoxelsWokeUp = a.NumVoxelsWokeUp + b.NumVoxelsWokeUp
    };
  }

  public void ResetFrame()
  {
    this.NumChunksProcessed = 0;
    this.NumChunksActive = 0;
    this.NumFlowEvents = 0;
    this.NumVoxelsProcessed = 0;
    this.NumVoxelsPutToSleep = 0;
    this.NumVoxelsWokeUp = 0;
  }
}
