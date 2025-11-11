// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.PathNodePool
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System.Collections.Generic;

#nullable disable
namespace WorldGenerationEngineFinal;

public class PathNodePool
{
  [PublicizedFrom(EAccessModifier.Private)]
  public List<PathNode> pool;
  [PublicizedFrom(EAccessModifier.Private)]
  public int used;

  public PathNodePool(int _initialSize) => this.pool = new List<PathNode>(_initialSize);

  public PathNode Alloc()
  {
    PathNode pathNode;
    if (this.used >= this.pool.Count)
    {
      pathNode = new PathNode();
      this.pool.Add(pathNode);
    }
    else
      pathNode = this.pool[this.used];
    ++this.used;
    return pathNode;
  }

  public void ReturnAll()
  {
    for (int index = 0; index < this.used; ++index)
      this.pool[index].Reset();
    this.used = 0;
  }

  public void Cleanup()
  {
    this.ReturnAll();
    this.pool.Clear();
    this.pool.Capacity = 16 /*0x10*/;
  }

  public void LogStats()
  {
    Log.Out($"PathNodePool: Capacity={this.pool.Capacity}, Allocated={this.pool.Count}, InUse={this.used}");
  }
}
