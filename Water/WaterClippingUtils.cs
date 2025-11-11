// Decompiled with JetBrains decompiler
// Type: WaterClippingUtils
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using System.Diagnostics;
using UnityEngine;

#nullable disable
public class WaterClippingUtils
{
  public const string PropWaterClipPlane = "WaterClipPlane";
  public const string PropWaterFlow = "WaterFlow";
  public static readonly Bounds CubeBounds = new Bounds(new Vector3(0.5f, 0.5f, 0.5f), Vector3.one);
  [PublicizedFrom(EAccessModifier.Private)]
  public static readonly Vector3[] cubeVerts = new Vector3[8]
  {
    new Vector3(0.0f, 0.0f, 0.0f),
    new Vector3(0.0f, 1f, 0.0f),
    new Vector3(1f, 1f, 0.0f),
    new Vector3(1f, 0.0f, 0.0f),
    new Vector3(0.0f, 0.0f, 1f),
    new Vector3(0.0f, 1f, 1f),
    new Vector3(1f, 1f, 1f),
    new Vector3(1f, 0.0f, 1f)
  };
  [PublicizedFrom(EAccessModifier.Private)]
  public static readonly int[] cubeEdges = new int[24]
  {
    0,
    1,
    1,
    2,
    2,
    3,
    3,
    0,
    4,
    5,
    5,
    6,
    6,
    7,
    7,
    4,
    0,
    4,
    1,
    5,
    2,
    6,
    3,
    7
  };
  [PublicizedFrom(EAccessModifier.Private)]
  public static readonly float[] cubeVertDistances = new float[8];
  [PublicizedFrom(EAccessModifier.Private)]
  public static readonly float[] hullVertAngles = new float[6];

  public static bool GetCubePlaneIntersectionEdgeLoop(
    Plane plane,
    ref Vector3[] intersectionPoints,
    out int count)
  {
    count = 0;
    for (int index = 0; index < WaterClippingUtils.cubeVerts.Length; ++index)
      WaterClippingUtils.cubeVertDistances[index] = plane.GetDistanceToPoint(WaterClippingUtils.cubeVerts[index]);
    Vector3 zero = Vector3.zero;
    for (int index = 0; index < WaterClippingUtils.cubeEdges.Length; index += 2)
    {
      float cubeVertDistance1 = WaterClippingUtils.cubeVertDistances[WaterClippingUtils.cubeEdges[index]];
      float cubeVertDistance2 = WaterClippingUtils.cubeVertDistances[WaterClippingUtils.cubeEdges[index + 1]];
      if ((double) Mathf.Sign(cubeVertDistance1) != (double) Mathf.Sign(cubeVertDistance2))
      {
        Vector3 cubeVert1 = WaterClippingUtils.cubeVerts[WaterClippingUtils.cubeEdges[index]];
        Vector3 cubeVert2 = WaterClippingUtils.cubeVerts[WaterClippingUtils.cubeEdges[index + 1]];
        float num = (float) (-(double) cubeVertDistance2 / ((double) cubeVertDistance1 - (double) cubeVertDistance2));
        Vector3 b = cubeVert1;
        double t = (double) num;
        Vector3 vector3 = Vector3.Lerp(cubeVert2, b, (float) t);
        intersectionPoints[count] = vector3;
        zero += vector3;
        ++count;
      }
    }
    if (count < 3)
      return false;
    Vector3 vector3_1 = zero / (float) count;
    Vector3 from = intersectionPoints[0] - vector3_1;
    WaterClippingUtils.hullVertAngles[0] = 0.0f;
    for (int index = 1; index < count; ++index)
    {
      float num = Vector3.SignedAngle(from, intersectionPoints[index] - vector3_1, plane.normal);
      WaterClippingUtils.hullVertAngles[index] = (double) num < 0.0 ? num + 360f : num;
    }
    for (int index = count; index < 6; ++index)
      WaterClippingUtils.hullVertAngles[index] = 1000f + (float) index;
    Array.Sort<float, Vector3>(WaterClippingUtils.hullVertAngles, intersectionPoints);
    return true;
  }

  [Conditional("DEBUG_WATER_CLIPPING")]
  [PublicizedFrom(EAccessModifier.Private)]
  public static void DebugDrawIntersectionSurface(
    Plane plane,
    Vector3[] intersectionPoints,
    int count,
    Vector3 hullCenter)
  {
    for (int index = 0; index < count; ++index)
    {
      Vector3 intersectionPoint = intersectionPoints[index];
      UnityEngine.Debug.DrawLine(intersectionPoint + 0.1f * Vector3.down, intersectionPoint + 0.1f * Vector3.up, Color.white);
      UnityEngine.Debug.DrawLine(intersectionPoint + 0.1f * Vector3.left, intersectionPoint + 0.1f * Vector3.right, Color.white);
      UnityEngine.Debug.DrawLine(intersectionPoint + 0.1f * Vector3.forward, intersectionPoint + 0.1f * Vector3.back, Color.white);
      UnityEngine.Debug.DrawLine(hullCenter, intersectionPoint, Color.white);
      UnityEngine.Debug.DrawLine(intersectionPoint, intersectionPoints[(index + 1) % count], Color.white);
    }
    for (int index = 0; index < WaterClippingUtils.cubeVerts.Length; ++index)
    {
      Vector3 vector3 = WaterClippingUtils.cubeVerts[index] + WaterClippingUtils.cubeVertDistances[index] * -plane.normal;
      if (!WaterClippingUtils.CubeBounds.Contains(vector3))
      {
        Vector3 end = GeometryUtils.NearestPointOnEdgeLoop(vector3, intersectionPoints, count);
        UnityEngine.Debug.DrawLine(vector3, end, Color.yellow);
      }
    }
  }

  [Conditional("DEBUG_WATER_CLIPPING")]
  [PublicizedFrom(EAccessModifier.Private)]
  public static void DebugDrawCubeVertPlaneOffsets(Plane plane)
  {
    for (int index = 0; index < WaterClippingUtils.cubeVerts.Length; ++index)
    {
      float cubeVertDistance = WaterClippingUtils.cubeVertDistances[index];
      UnityEngine.Debug.DrawRay(WaterClippingUtils.cubeVerts[index], cubeVertDistance * -plane.normal, (double) Mathf.Sign(cubeVertDistance) > 0.0 ? Color.green : Color.red);
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  static WaterClippingUtils()
  {
  }
}
