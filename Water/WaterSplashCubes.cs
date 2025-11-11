// Decompiled with JetBrains decompiler
// Type: WaterSplashCubes
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System.Collections;
using System.Collections.Generic;
using UnityEngine;

#nullable disable
public class WaterSplashCubes
{
  public static WaterSplashCubes instance = (WaterSplashCubes) null;
  [PublicizedFrom(EAccessModifier.Private)]
  public static DictionaryList<long, DictionaryList<int, GameObject>> splashes = new DictionaryList<long, DictionaryList<int, GameObject>>();
  [PublicizedFrom(EAccessModifier.Private)]
  public static GameObject root;
  public static List<WaterSplashCubes.ParticlePlacement> addList;
  public static List<Vector3i> removeList;
  [PublicizedFrom(EAccessModifier.Private)]
  public static int checkListNum = 0;
  [PublicizedFrom(EAccessModifier.Private)]
  public static Object[] waterFallSplashCubeEffect = (Object[]) null;
  public static float particleLimiter = 1f;
  [PublicizedFrom(EAccessModifier.Private)]
  public static int particleCount = 0;
  [PublicizedFrom(EAccessModifier.Private)]
  public static float cleanUpTimer = 0.0f;
  [PublicizedFrom(EAccessModifier.Private)]
  public static int currentCleanupIndex = 15;

  public WaterSplashCubes()
  {
    if (WaterSplashCubes.instance == null)
      WaterSplashCubes.instance = this;
    WaterSplashCubes.addList = new List<WaterSplashCubes.ParticlePlacement>();
    WaterSplashCubes.removeList = new List<Vector3i>();
    WaterSplashCubes.root = GameObject.Find("WaterSplashes");
    WaterSplashCubes.particleLimiter = GamePrefs.GetFloat(EnumGamePrefs.OptionsGfxWaterPtlLimiter);
  }

  public static long MakeKey(int x, int z)
  {
    return ((long) z & 16777215L /*0xFFFFFF*/) << 24 | (long) x & 16777215L /*0xFFFFFF*/;
  }

  public static object GetSyncRoot() => ((ICollection) WaterSplashCubes.splashes.dict).SyncRoot;

  [PublicizedFrom(EAccessModifier.Private)]
  public static GameObject AddParticleEffect(
    Vector3i pos,
    BlockFace face,
    WaterSplashCubes.SplashType type)
  {
    if (WaterSplashCubes.waterFallSplashCubeEffect == null)
    {
      WaterSplashCubes.waterFallSplashCubeEffect = new Object[EnumUtils.Names<WaterSplashCubes.SplashType>().Count];
      WaterSplashCubes.waterFallSplashCubeEffect[0] = Resources.Load("prefabs/WaterFallSplashCube");
      WaterSplashCubes.waterFallSplashCubeEffect[1] = Resources.Load("prefabs/WaterFallSlopeParticles");
      WaterSplashCubes.waterFallSplashCubeEffect[2] = Resources.Load("prefabs/WaterFallAreaParticles");
      WaterSplashCubes.waterFallSplashCubeEffect[3] = Resources.Load("prefabs/WaterFallSplashCube");
    }
    GameObject gameObject = (GameObject) Object.Instantiate(WaterSplashCubes.waterFallSplashCubeEffect[(int) type]);
    ++WaterSplashCubes.particleCount;
    switch (face)
    {
      case BlockFace.Top:
        gameObject.transform.Rotate(new Vector3(90f, 0.0f, 0.0f));
        break;
      case BlockFace.Bottom:
        gameObject.transform.Rotate(new Vector3(-90f, 0.0f, 0.0f));
        break;
      case BlockFace.North:
        gameObject.transform.Rotate(new Vector3(0.0f, 0.0f, 0.0f));
        break;
      case BlockFace.West:
        gameObject.transform.Rotate(new Vector3(0.0f, -90f, 0.0f));
        break;
      case BlockFace.South:
        gameObject.transform.Rotate(new Vector3(0.0f, 180f, 0.0f));
        break;
      case BlockFace.East:
        gameObject.transform.Rotate(new Vector3(0.0f, 90f, 0.0f));
        break;
    }
    gameObject.transform.position = new Vector3((float) pos.x + 0.5f, (float) pos.y + 0.5f, (float) pos.z + 0.5f) - Origin.position;
    gameObject.transform.parent = WaterSplashCubes.root.transform;
    if ((double) WaterSplashCubes.particleLimiter < 1.0 && WaterSplashCubes.particleCount % Mathf.CeilToInt((float) ((1.0 - (double) WaterSplashCubes.particleLimiter) * 6.0)) != 0)
      gameObject.SetActive(false);
    return gameObject;
  }

  public static void Update()
  {
    if (WaterSplashCubes.removeList == null || WaterSplashCubes.addList == null)
      return;
    lock (WaterSplashCubes.removeList)
    {
      for (int index = 0; index < WaterSplashCubes.removeList.Count; ++index)
      {
        Vector3i remove = WaterSplashCubes.removeList[index];
        int x = remove.x;
        int y = remove.y;
        int z = remove.z;
        long key = WaterSplashCubes.MakeKey(x, z);
        lock (WaterSplashCubes.GetSyncRoot())
        {
          if (WaterSplashCubes.splashes.dict.ContainsKey(key))
          {
            DictionaryList<int, GameObject> dictionaryList = WaterSplashCubes.splashes.dict[key];
            if (dictionaryList.dict.ContainsKey(y))
            {
              Object.DestroyImmediate((Object) dictionaryList.dict[y]);
              dictionaryList.Remove(y);
            }
          }
        }
      }
      WaterSplashCubes.removeList.Clear();
    }
    lock (WaterSplashCubes.addList)
    {
      for (int index = 0; index < WaterSplashCubes.addList.Count; ++index)
      {
        WaterSplashCubes.ParticlePlacement add = WaterSplashCubes.addList[index];
        int x = add.pos.x;
        int y = add.pos.y;
        int z = add.pos.z;
        long num = WaterSplashCubes.MakeKey(x, z);
        lock (WaterSplashCubes.GetSyncRoot())
        {
          if (!WaterSplashCubes.splashes.dict.ContainsKey(num))
          {
            DictionaryList<int, GameObject> dictionaryList = new DictionaryList<int, GameObject>();
            dictionaryList.Add(y, WaterSplashCubes.AddParticleEffect(add.pos, add.dir, add.type));
            WaterSplashCubes.splashes.Add(num, dictionaryList);
          }
          else
          {
            DictionaryList<int, GameObject> dictionaryList = WaterSplashCubes.splashes.dict[num];
            if (!dictionaryList.dict.ContainsKey(y))
              dictionaryList.Add(y, WaterSplashCubes.AddParticleEffect(add.pos, add.dir, add.type));
          }
        }
      }
      WaterSplashCubes.addList.Clear();
    }
    WaterSplashCubes.CleanUp();
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static void CleanUp()
  {
    if ((double) Time.time <= (double) WaterSplashCubes.cleanUpTimer + 0.15000000596046448)
      return;
    WaterSplashCubes.cleanUpTimer = Time.time;
    if ((Object) WaterSplashCubes.root == (Object) null || (Object) WaterSplashCubes.root.transform == (Object) null || (Object) GameManager.Instance == (Object) null)
      return;
    World world = GameManager.Instance.World;
    if (world == null)
      return;
    bool flag = false;
    if (WaterSplashCubes.currentCleanupIndex >= WaterSplashCubes.root.transform.childCount)
    {
      WaterSplashCubes.currentCleanupIndex = WaterSplashCubes.root.transform.childCount;
      flag = true;
    }
    for (int index = 0; index < WaterSplashCubes.root.transform.childCount && index < WaterSplashCubes.currentCleanupIndex; ++index)
    {
      Transform child = WaterSplashCubes.root.transform.GetChild(index);
      if ((Object) child != (Object) null)
      {
        Vector3i _pos = new Vector3i((int) ((double) child.position.x - 0.5), (int) ((double) child.position.y - 0.5), (int) ((double) child.position.z - 0.5));
        if (!world.IsChunkAreaLoaded(_pos.x, _pos.y, _pos.z))
          WaterSplashCubes.RemoveSplashAt(_pos.x, _pos.y, _pos.z);
        else if (!world.IsWater(_pos) || !world.IsAir((int) ((double) child.position.x - 0.5), (int) ((double) child.position.y - 0.5) + 1, (int) ((double) child.position.z - 0.5)))
          WaterSplashCubes.RemoveSplashAt(_pos.x, _pos.y, _pos.z);
      }
    }
    WaterSplashCubes.currentCleanupIndex += 15;
    if (!flag)
      return;
    WaterSplashCubes.currentCleanupIndex = 0;
  }

  public static void RemoveSplashAt(int _x, int _y, int _z)
  {
    if (WaterSplashCubes.removeList == null)
      return;
    lock (WaterSplashCubes.removeList)
      WaterSplashCubes.removeList.Add(new Vector3i(_x, _y, _z));
  }

  public static void AddSplashAt(
    int _x,
    int _y,
    int _z,
    BlockFace _dir,
    WaterSplashCubes.SplashType _type)
  {
    if ((double) WaterSplashCubes.particleLimiter <= 0.0 || WaterSplashCubes.addList == null)
      return;
    lock (WaterSplashCubes.addList)
    {
      WaterSplashCubes.ParticlePlacement particlePlacement = new WaterSplashCubes.ParticlePlacement(new Vector3i(_x, _y, _z), _dir, _type);
      WaterSplashCubes.addList.Add(particlePlacement);
    }
  }

  public static void Clear()
  {
    lock (WaterSplashCubes.GetSyncRoot())
    {
      for (int index1 = 0; index1 < WaterSplashCubes.splashes.Count; ++index1)
      {
        DictionaryList<int, GameObject> dictionaryList = WaterSplashCubes.splashes.list[index1];
        for (int index2 = 0; index2 < dictionaryList.list.Count; ++index2)
          Object.DestroyImmediate((Object) dictionaryList.list[index2]);
        dictionaryList.Clear();
      }
      WaterSplashCubes.splashes.Clear();
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  static WaterSplashCubes()
  {
  }

  public enum SplashType
  {
    Splash,
    Slope,
    Area,
    Mist,
  }

  public class ParticlePlacement
  {
    public Vector3i pos;
    public BlockFace dir;
    public WaterSplashCubes.SplashType type;

    public ParticlePlacement(Vector3i _pos, BlockFace _dir, WaterSplashCubes.SplashType _type)
    {
      this.pos = _pos;
      this.dir = _dir;
      this.type = _type;
    }
  }
}
