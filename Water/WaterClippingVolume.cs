// Decompiled with JetBrains decompiler
// Type: WaterClippingVolume
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using UnityEngine;

#nullable disable
public class WaterClippingVolume
{
  [PublicizedFrom(EAccessModifier.Private)]
  public Plane waterClipPlane;
  [PublicizedFrom(EAccessModifier.Private)]
  public Vector3[] intersectionPoints = new Vector3[6];
  [PublicizedFrom(EAccessModifier.Private)]
  public int count = -1;
  [PublicizedFrom(EAccessModifier.Private)]
  public bool isSliced;

  public void Prepare(Plane waterClipPlane)
  {
    this.waterClipPlane = waterClipPlane;
    this.isSliced = WaterClippingUtils.GetCubePlaneIntersectionEdgeLoop(waterClipPlane, ref this.intersectionPoints, out this.count);
  }

  public void ApplyClipping(ref Vector3 vertLocalPos)
  {
    if (!this.isSliced || (double) this.waterClipPlane.GetDistanceToPoint(vertLocalPos) <= 0.0)
      return;
    vertLocalPos = this.waterClipPlane.ClosestPointOnPlane(vertLocalPos);
    if (WaterClippingUtils.CubeBounds.Contains(vertLocalPos))
      return;
    vertLocalPos = GeometryUtils.NearestPointOnEdgeLoop(vertLocalPos, this.intersectionPoints, this.count);
  }
}
