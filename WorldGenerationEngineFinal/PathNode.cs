// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.PathNode
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

#nullable disable
namespace WorldGenerationEngineFinal;

public class PathNode
{
  public Vector2i position;
  public float pathCost;
  public PathNode next;
  public PathNode nextListElem;

  public PathNode(Vector2i position, float pathCost, PathNode next)
  {
    this.position = position;
    this.pathCost = pathCost;
    this.next = next;
  }

  public PathNode()
  {
  }

  public void Set(Vector2i position, float pathCost, PathNode next)
  {
    this.position = position;
    this.pathCost = pathCost;
    this.next = next;
  }

  public void Reset()
  {
    this.next = (PathNode) null;
    this.nextListElem = (PathNode) null;
  }
}
