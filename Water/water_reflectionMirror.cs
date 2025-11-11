// Decompiled with JetBrains decompiler
// Type: water_reflectionMirror
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using System.Collections;
using UnityEngine;

#nullable disable
[ExecuteInEditMode]
public class water_reflectionMirror : MonoBehaviour
{
  public bool m_DisablePixelLights = true;
  public int m_TextureSize = 256 /*0x0100*/;
  public float m_ClipPlaneOffset = 0.07f;
  public LayerMask m_ReflectLayers = (LayerMask) -1;
  [PublicizedFrom(EAccessModifier.Private)]
  [NonSerialized]
  public Hashtable m_ReflectionCameras = new Hashtable();
  public static RenderTexture m_ReflectionTexture;
  [PublicizedFrom(EAccessModifier.Private)]
  [NonSerialized]
  public int m_OldReflectionTextureSize;
  [PublicizedFrom(EAccessModifier.Private)]
  [NonSerialized]
  public static bool s_InsideRendering;

  public void OnWillRenderObject()
  {
    if (!this.enabled || !(bool) (UnityEngine.Object) this.GetComponent<Renderer>() || !(bool) (UnityEngine.Object) this.GetComponent<Renderer>().sharedMaterial || !this.GetComponent<Renderer>().enabled)
      return;
    Camera current = Camera.current;
    if (!(bool) (UnityEngine.Object) current || water_reflectionMirror.s_InsideRendering)
      return;
    water_reflectionMirror.s_InsideRendering = true;
    Camera reflectionCamera;
    this.CreateMirrorObjects(current, out reflectionCamera);
    Vector3 position1 = this.transform.position;
    Vector3 up = this.transform.up;
    int pixelLightCount = QualitySettings.pixelLightCount;
    if (this.m_DisablePixelLights)
      QualitySettings.pixelLightCount = 0;
    this.UpdateCameraModes(current, reflectionCamera);
    float w = -Vector3.Dot(up, position1) - this.m_ClipPlaneOffset;
    Vector4 plane = new Vector4(up.x, up.y, up.z, w);
    Matrix4x4 zero = Matrix4x4.zero;
    water_reflectionMirror.CalculateReflectionMatrix(ref zero, plane);
    Vector3 position2 = current.transform.position;
    Vector3 vector3 = zero.MultiplyPoint(position2);
    reflectionCamera.worldToCameraMatrix = current.worldToCameraMatrix * zero;
    Vector4 clipPlane = this.CameraSpacePlane(reflectionCamera, position1, up, 1f);
    Matrix4x4 projectionMatrix1 = current.projectionMatrix;
    water_reflectionMirror.CalculateObliqueMatrix(ref projectionMatrix1, clipPlane);
    reflectionCamera.projectionMatrix = projectionMatrix1;
    reflectionCamera.cullingMask = -17 & this.m_ReflectLayers.value;
    reflectionCamera.targetTexture = water_reflectionMirror.m_ReflectionTexture;
    GL.invertCulling = true;
    reflectionCamera.transform.position = vector3;
    Vector3 eulerAngles = current.transform.eulerAngles;
    reflectionCamera.transform.eulerAngles = new Vector3(0.0f, eulerAngles.y, eulerAngles.z);
    reflectionCamera.Render();
    reflectionCamera.transform.position = position2;
    GL.invertCulling = false;
    Material[] sharedMaterials = this.GetComponent<Renderer>().sharedMaterials;
    foreach (Material material in sharedMaterials)
    {
      if (material.HasProperty("_ReflectionTex"))
        material.SetTexture("_ReflectionTex", (Texture) water_reflectionMirror.m_ReflectionTexture);
    }
    Matrix4x4 matrix4x4_1 = Matrix4x4.TRS(new Vector3(0.5f, 0.5f, 0.5f), Quaternion.identity, new Vector3(0.5f, 0.5f, 0.5f));
    Vector3 lossyScale = this.transform.lossyScale;
    Matrix4x4 matrix4x4_2 = this.transform.localToWorldMatrix * Matrix4x4.Scale(new Vector3(1f / lossyScale.x, 1f / lossyScale.y, 1f / lossyScale.z));
    Matrix4x4 projectionMatrix2 = current.projectionMatrix;
    Matrix4x4 matrix4x4_3 = matrix4x4_1 * projectionMatrix2 * current.worldToCameraMatrix * matrix4x4_2;
    foreach (Material material in sharedMaterials)
      material.SetMatrix("_ProjMatrix", matrix4x4_3);
    if (this.m_DisablePixelLights)
      QualitySettings.pixelLightCount = pixelLightCount;
    water_reflectionMirror.s_InsideRendering = false;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void OnDisable()
  {
    if ((bool) (UnityEngine.Object) water_reflectionMirror.m_ReflectionTexture)
    {
      UnityEngine.Object.DestroyImmediate((UnityEngine.Object) water_reflectionMirror.m_ReflectionTexture);
      water_reflectionMirror.m_ReflectionTexture = (RenderTexture) null;
    }
    foreach (DictionaryEntry reflectionCamera in this.m_ReflectionCameras)
      UnityEngine.Object.DestroyImmediate((UnityEngine.Object) ((Component) reflectionCamera.Value).gameObject);
    this.m_ReflectionCameras.Clear();
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void UpdateCameraModes(Camera src, Camera dest)
  {
    if ((UnityEngine.Object) dest == (UnityEngine.Object) null)
      return;
    dest.clearFlags = src.clearFlags;
    dest.renderingPath = RenderingPath.Forward;
    dest.backgroundColor = src.backgroundColor;
    if (src.clearFlags == CameraClearFlags.Skybox)
    {
      Skybox component1 = src.GetComponent(typeof (Skybox)) as Skybox;
      Skybox component2 = dest.GetComponent(typeof (Skybox)) as Skybox;
      if (!(bool) (UnityEngine.Object) component1 || !(bool) (UnityEngine.Object) component1.material)
      {
        component2.enabled = false;
      }
      else
      {
        component2.enabled = true;
        component2.material = component1.material;
      }
    }
    dest.farClipPlane = src.farClipPlane;
    dest.nearClipPlane = src.nearClipPlane;
    dest.orthographic = src.orthographic;
    dest.fieldOfView = src.fieldOfView;
    dest.aspect = src.aspect;
    dest.orthographicSize = src.orthographicSize;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void CreateMirrorObjects(Camera currentCamera, out Camera reflectionCamera)
  {
    reflectionCamera = (Camera) null;
    if (!(bool) (UnityEngine.Object) water_reflectionMirror.m_ReflectionTexture || this.m_OldReflectionTextureSize != this.m_TextureSize)
    {
      if ((bool) (UnityEngine.Object) water_reflectionMirror.m_ReflectionTexture)
        UnityEngine.Object.DestroyImmediate((UnityEngine.Object) water_reflectionMirror.m_ReflectionTexture);
      water_reflectionMirror.m_ReflectionTexture = new RenderTexture(this.m_TextureSize, this.m_TextureSize, 16 /*0x10*/);
      water_reflectionMirror.m_ReflectionTexture.name = "__MirrorReflection" + this.GetInstanceID().ToString();
      water_reflectionMirror.m_ReflectionTexture.isPowerOfTwo = true;
      water_reflectionMirror.m_ReflectionTexture.hideFlags = HideFlags.DontSave;
      this.m_OldReflectionTextureSize = this.m_TextureSize;
    }
    reflectionCamera = this.m_ReflectionCameras[(object) currentCamera] as Camera;
    if ((bool) (UnityEngine.Object) reflectionCamera)
      return;
    GameObject gameObject = new GameObject($"Mirror Refl Camera id{this.GetInstanceID().ToString()} for {currentCamera.GetInstanceID().ToString()}", new System.Type[2]
    {
      typeof (Camera),
      typeof (Skybox)
    });
    reflectionCamera = gameObject.GetComponent<Camera>();
    reflectionCamera.enabled = false;
    reflectionCamera.transform.position = this.transform.position;
    reflectionCamera.transform.rotation = this.transform.rotation;
    reflectionCamera.gameObject.AddComponent<FlareLayer>();
    gameObject.hideFlags = HideFlags.HideAndDontSave;
    this.m_ReflectionCameras[(object) currentCamera] = (object) reflectionCamera;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static float sgn(float a)
  {
    if ((double) a > 0.0)
      return 1f;
    return (double) a < 0.0 ? -1f : 0.0f;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public Vector4 CameraSpacePlane(Camera cam, Vector3 pos, Vector3 normal, float sideSign)
  {
    Vector3 point = pos + normal * this.m_ClipPlaneOffset;
    Matrix4x4 worldToCameraMatrix = cam.worldToCameraMatrix;
    Vector3 lhs = worldToCameraMatrix.MultiplyPoint(point);
    Vector3 rhs = worldToCameraMatrix.MultiplyVector(normal).normalized * sideSign;
    return new Vector4(rhs.x, rhs.y, rhs.z, -Vector3.Dot(lhs, rhs));
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static void CalculateObliqueMatrix(ref Matrix4x4 projection, Vector4 clipPlane)
  {
    Vector4 b = projection.inverse * new Vector4(water_reflectionMirror.sgn(clipPlane.x), water_reflectionMirror.sgn(clipPlane.y), 1f, 1f);
    Vector4 vector4 = clipPlane * (2f / Vector4.Dot(clipPlane, b));
    projection[2] = vector4.x - projection[3];
    projection[6] = vector4.y - projection[7];
    projection[10] = vector4.z - projection[11];
    projection[14] = vector4.w - projection[15];
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static void CalculateReflectionMatrix(ref Matrix4x4 reflectionMat, Vector4 plane)
  {
    reflectionMat.m00 = (float) (1.0 - 2.0 * (double) plane[0] * (double) plane[0]);
    reflectionMat.m01 = -2f * plane[0] * plane[1];
    reflectionMat.m02 = -2f * plane[0] * plane[2];
    reflectionMat.m03 = -2f * plane[3] * plane[0];
    reflectionMat.m10 = -2f * plane[1] * plane[0];
    reflectionMat.m11 = (float) (1.0 - 2.0 * (double) plane[1] * (double) plane[1]);
    reflectionMat.m12 = -2f * plane[1] * plane[2];
    reflectionMat.m13 = -2f * plane[3] * plane[1];
    reflectionMat.m20 = -2f * plane[2] * plane[0];
    reflectionMat.m21 = -2f * plane[2] * plane[1];
    reflectionMat.m22 = (float) (1.0 - 2.0 * (double) plane[2] * (double) plane[2]);
    reflectionMat.m23 = -2f * plane[3] * plane[2];
    reflectionMat.m30 = 0.0f;
    reflectionMat.m31 = 0.0f;
    reflectionMat.m32 = 0.0f;
    reflectionMat.m33 = 1f;
  }
}
