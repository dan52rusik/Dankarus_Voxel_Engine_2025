// Decompiled with JetBrains decompiler
// Type: WaterStatsProfiler
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System.Diagnostics;
using Unity.Profiling;

#nullable disable
public static class WaterStatsProfiler
{
  [PublicizedFrom(EAccessModifier.Private)]
  public static readonly ProfilerCounter<int> NumChunksProcessed = new ProfilerCounter<int>(ProfilerCategory.Scripts, "Num Chunks Processed", ProfilerMarkerDataUnit.Count);
  [PublicizedFrom(EAccessModifier.Private)]
  public static readonly ProfilerCounter<int> NumChunksActive = new ProfilerCounter<int>(ProfilerCategory.Scripts, "Num Chunks Active", ProfilerMarkerDataUnit.Count);
  [PublicizedFrom(EAccessModifier.Private)]
  public static readonly ProfilerCounter<int> NumFlowEvents = new ProfilerCounter<int>(ProfilerCategory.Scripts, "Num Flow Events", ProfilerMarkerDataUnit.Count);
  [PublicizedFrom(EAccessModifier.Private)]
  public static readonly ProfilerCounter<int> NumVoxelProcessed = new ProfilerCounter<int>(ProfilerCategory.Scripts, "Num Voxels Processed", ProfilerMarkerDataUnit.Count);
  [PublicizedFrom(EAccessModifier.Private)]
  public static readonly ProfilerCounter<int> NumVoxelsPutToSleep = new ProfilerCounter<int>(ProfilerCategory.Scripts, "Num Voxels Put To Sleep", ProfilerMarkerDataUnit.Count);
  [PublicizedFrom(EAccessModifier.Private)]
  public static readonly ProfilerCounter<int> NumVoxelsWokeUp = new ProfilerCounter<int>(ProfilerCategory.Scripts, "Num Voxels Woke Up", ProfilerMarkerDataUnit.Count);

  public static void SampleTick(WaterStats stats)
  {
  }

  [PublicizedFrom(EAccessModifier.Private)]
  static WaterStatsProfiler()
  {
  }

  public struct Timer(string name)
  {
    [PublicizedFrom(EAccessModifier.Private)]
    public static readonly double tickToNanos = 1000000000.0 / (double) Stopwatch.Frequency;
    [PublicizedFrom(EAccessModifier.Private)]
    public Stopwatch stopwatch = new Stopwatch();
    public ProfilerCounter<double> counterValue = new ProfilerCounter<double>(ProfilerCategory.Scripts, name, ProfilerMarkerDataUnit.TimeNanoseconds);

    public void Start() => this.stopwatch.Start();

    public void Stop() => this.stopwatch.Stop();

    public void Sample() => this.stopwatch.Reset();

    [PublicizedFrom(EAccessModifier.Private)]
    static Timer()
    {
    }
  }
}
