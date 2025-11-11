// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.PathTile
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

#nullable disable
namespace WorldGenerationEngineFinal;

public class PathTile
{
  public PathTile.PathTileStates TileState;
  public byte PathRadius;
  public Path Path;

  public enum PathTileStates : byte
  {
    Free,
    Blocked,
    Highway,
    Country,
  }
}
