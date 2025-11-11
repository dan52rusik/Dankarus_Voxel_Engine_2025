// Decompiled with JetBrains decompiler
// Type: WaterDebug
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System.Diagnostics;
using System.Runtime.CompilerServices;

#nullable disable
public static class WaterDebug
{
  [CompilerGenerated]
  [PublicizedFrom(EAccessModifier.Private)]
  public static WaterDebugManager \u003CManager\u003Ek__BackingField;

  public static WaterDebugManager Manager
  {
    [PublicizedFrom(EAccessModifier.Private)] get => WaterDebug.\u003CManager\u003Ek__BackingField;
    [PublicizedFrom(EAccessModifier.Private)] set
    {
      WaterDebug.\u003CManager\u003Ek__BackingField = value;
    }
  }

  public static bool IsAvailable => WaterDebug.Manager != null;

  public static bool RenderingEnabled
  {
    get
    {
      WaterDebugManager manager = WaterDebug.Manager;
      return manager != null && manager.RenderingEnabled;
    }
    set
    {
      if (WaterDebug.Manager == null)
        return;
      WaterDebug.Manager.RenderingEnabled = value;
    }
  }

  [Conditional("UNITY_EDITOR")]
  public static void Init()
  {
    if (!WaterSimulationNative.Instance.ShouldEnable)
      return;
    WaterDebugPools.CreatePools();
    WaterDebug.Manager = new WaterDebugManager();
    WaterDebug.RenderingEnabled = false;
  }

  [Conditional("UNITY_EDITOR")]
  public static void InitializeForChunk(Chunk _chunk)
  {
    WaterDebug.Manager?.InitializeDebugRender(_chunk);
  }

  [Conditional("UNITY_EDITOR")]
  public static void Draw() => WaterDebug.Manager?.DebugDraw();

  [Conditional("UNITY_EDITOR")]
  public static void Cleanup()
  {
    WaterDebug.Manager?.Cleanup();
    WaterDebugPools.Cleanup();
    WaterDebug.Manager = (WaterDebugManager) null;
  }
}
