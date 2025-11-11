// Decompiled with JetBrains decompiler
// Type: WaterEvaporationManager
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using System.Collections.Generic;
using UnityEngine;

#nullable disable
public class WaterEvaporationManager : MonoBehaviour
{
  [PublicizedFrom(EAccessModifier.Private)]
  [NonSerialized]
  public static int evapWalkIndex = 0;
  [PublicizedFrom(EAccessModifier.Private)]
  [NonSerialized]
  public static ulong uniqueIndex = 0;
  [PublicizedFrom(EAccessModifier.Private)]
  [NonSerialized]
  public static int restWalkIndex = 0;
  [PublicizedFrom(EAccessModifier.Private)]
  [NonSerialized]
  public static DictionaryList<ulong, WaterEvaporationManager.BlockData> evapWalkList = new DictionaryList<ulong, WaterEvaporationManager.BlockData>();
  [PublicizedFrom(EAccessModifier.Private)]
  [NonSerialized]
  public static DictionaryList<int, DictionaryList<int, DictionaryList<int, WaterEvaporationManager.BlockData>>> evaporationList = new DictionaryList<int, DictionaryList<int, DictionaryList<int, WaterEvaporationManager.BlockData>>>();
  [PublicizedFrom(EAccessModifier.Private)]
  [NonSerialized]
  public static DictionaryList<ulong, WaterEvaporationManager.BlockData> restWalkList = new DictionaryList<ulong, WaterEvaporationManager.BlockData>();
  [PublicizedFrom(EAccessModifier.Private)]
  [NonSerialized]
  public static DictionaryList<int, DictionaryList<int, DictionaryList<int, WaterEvaporationManager.BlockData>>> restingList = new DictionaryList<int, DictionaryList<int, DictionaryList<int, WaterEvaporationManager.BlockData>>>();
  [PublicizedFrom(EAccessModifier.Private)]
  [NonSerialized]
  public static int waterBlockID = -1;
  [PublicizedFrom(EAccessModifier.Private)]
  [NonSerialized]
  public static List<Vector3i> vRemovalList = new List<Vector3i>();
  [PublicizedFrom(EAccessModifier.Private)]
  [NonSerialized]
  public static List<ulong> uRemovalList = new List<ulong>();

  public void Start()
  {
  }

  public void Update()
  {
  }

  public static void UpdateEvaporation()
  {
    if ((UnityEngine.Object) GameManager.Instance == (UnityEngine.Object) null || GameManager.Instance.World == null)
      return;
    WaterEvaporationManager.vRemovalList.Clear();
    WaterEvaporationManager.uRemovalList.Clear();
    lock (WaterEvaporationManager.evapWalkList)
    {
      for (int evapWalkIndex = WaterEvaporationManager.evapWalkIndex; evapWalkIndex < WaterEvaporationManager.evapWalkList.Count && evapWalkIndex < WaterEvaporationManager.evapWalkIndex + 15; ++evapWalkIndex)
      {
        if ((UnityEngine.Object) GameManager.Instance == (UnityEngine.Object) null || GameManager.Instance.World == null)
          return;
        if (GameManager.Instance.World.GetWorldTime() > WaterEvaporationManager.evapWalkList.list[evapWalkIndex].time + 1000UL)
        {
          WaterEvaporationManager.vRemovalList.Add(WaterEvaporationManager.evapWalkList.list[evapWalkIndex].pos);
          WaterEvaporationManager.uRemovalList.Add(WaterEvaporationManager.evapWalkList.list[evapWalkIndex].ID);
          GameManager.Instance.World.SetBlockRPC(WaterEvaporationManager.evapWalkList.list[evapWalkIndex].pos, BlockValue.Air);
          if (WaterEvaporationManager.waterBlockID < 0)
            WaterEvaporationManager.waterBlockID = ItemClass.GetItem("Water").ToBlockValue().type;
          GameManager.Instance.World.GetWBT().AddScheduledBlockUpdate(0, WaterEvaporationManager.evapWalkList.list[evapWalkIndex].pos, WaterEvaporationManager.waterBlockID, 1UL);
        }
      }
      int num = 0;
      for (int index = 0; index < WaterEvaporationManager.vRemovalList.Count; ++index)
      {
        Vector3i vRemoval = WaterEvaporationManager.vRemovalList[index];
        WaterEvaporationManager.evapWalkList.Remove(WaterEvaporationManager.uRemovalList[num++]);
        WaterEvaporationManager.RemoveFromEvapList(vRemoval);
      }
      WaterEvaporationManager.evapWalkIndex += 15;
      if (WaterEvaporationManager.evapWalkIndex >= WaterEvaporationManager.evapWalkList.Count)
        WaterEvaporationManager.evapWalkIndex = 0;
    }
    WaterEvaporationManager.vRemovalList.Clear();
    WaterEvaporationManager.uRemovalList.Clear();
    lock (WaterEvaporationManager.restWalkList)
    {
      for (int restWalkIndex = WaterEvaporationManager.restWalkIndex; restWalkIndex < WaterEvaporationManager.restWalkList.Count && restWalkIndex < WaterEvaporationManager.restWalkIndex + 15; ++restWalkIndex)
      {
        if ((UnityEngine.Object) GameManager.Instance == (UnityEngine.Object) null || GameManager.Instance.World == null)
          return;
        if (GameManager.Instance.World.GetWorldTime() > WaterEvaporationManager.restWalkList.list[restWalkIndex].time + 1000UL)
        {
          WaterEvaporationManager.vRemovalList.Add(WaterEvaporationManager.restWalkList.list[restWalkIndex].pos);
          WaterEvaporationManager.uRemovalList.Add(WaterEvaporationManager.restWalkList.list[restWalkIndex].ID);
          BlockValue block = GameManager.Instance.World.GetBlock(WaterEvaporationManager.restWalkList.list[restWalkIndex].pos.x, WaterEvaporationManager.restWalkList.list[restWalkIndex].pos.y, WaterEvaporationManager.restWalkList.list[restWalkIndex].pos.z);
          BlockLiquidv2.SetBlockState(ref block, BlockLiquidv2.UpdateID.Evaporate);
          GameManager.Instance.World.SetBlockRPC(WaterEvaporationManager.restWalkList.list[restWalkIndex].pos, block);
          if (WaterEvaporationManager.waterBlockID < 0)
            WaterEvaporationManager.waterBlockID = ItemClass.GetItem("Water").ToBlockValue().type;
          GameManager.Instance.World.GetWBT().AddScheduledBlockUpdate(0, WaterEvaporationManager.restWalkList.list[restWalkIndex].pos, WaterEvaporationManager.waterBlockID, 1UL);
        }
      }
      int num = 0;
      for (int index = 0; index < WaterEvaporationManager.vRemovalList.Count; ++index)
      {
        Vector3i vRemoval = WaterEvaporationManager.vRemovalList[index];
        WaterEvaporationManager.restWalkList.Remove(WaterEvaporationManager.uRemovalList[num++]);
        WaterEvaporationManager.RemoveFromRestList(vRemoval);
      }
      WaterEvaporationManager.restWalkIndex += 15;
      if (WaterEvaporationManager.restWalkIndex < WaterEvaporationManager.restWalkList.Count)
        return;
      WaterEvaporationManager.restWalkIndex = 0;
    }
  }

  public static void ClearAll()
  {
    lock (WaterEvaporationManager.restingList)
      WaterEvaporationManager.restingList.Clear();
    lock (WaterEvaporationManager.evaporationList)
      WaterEvaporationManager.evaporationList.Clear();
    lock (WaterEvaporationManager.evapWalkList)
      WaterEvaporationManager.evapWalkList.Clear();
    lock (WaterEvaporationManager.restWalkList)
      WaterEvaporationManager.restWalkList.Clear();
  }

  public static void AddToRestList(Vector3i _blockPos)
  {
    lock (WaterEvaporationManager.restingList)
    {
      lock (WaterEvaporationManager.restWalkList)
      {
        if (WaterEvaporationManager.restingList.dict.ContainsKey(_blockPos.x))
        {
          if (WaterEvaporationManager.restingList.dict[_blockPos.x].dict.ContainsKey(_blockPos.y))
          {
            if (WaterEvaporationManager.restingList.dict[_blockPos.x].dict[_blockPos.y].dict.ContainsKey(_blockPos.z))
            {
              WaterEvaporationManager.BlockData blockData = new WaterEvaporationManager.BlockData(_blockPos);
              WaterEvaporationManager.restingList.dict[_blockPos.x].dict[_blockPos.y].dict[_blockPos.z] = blockData;
              WaterEvaporationManager.restWalkList.Add(blockData.ID, blockData);
            }
            else
            {
              WaterEvaporationManager.BlockData blockData = new WaterEvaporationManager.BlockData(_blockPos);
              WaterEvaporationManager.restingList.dict[_blockPos.x].dict[_blockPos.y].Add(_blockPos.z, blockData);
              WaterEvaporationManager.restWalkList.Add(blockData.ID, blockData);
            }
          }
          else
          {
            WaterEvaporationManager.BlockData blockData = new WaterEvaporationManager.BlockData(_blockPos);
            WaterEvaporationManager.restingList.dict[_blockPos.x].Add(_blockPos.y, new DictionaryList<int, WaterEvaporationManager.BlockData>());
            WaterEvaporationManager.restingList.dict[_blockPos.x].dict[_blockPos.y].Add(_blockPos.z, blockData);
            WaterEvaporationManager.restWalkList.Add(blockData.ID, blockData);
          }
        }
        else
        {
          WaterEvaporationManager.BlockData blockData = new WaterEvaporationManager.BlockData(_blockPos);
          WaterEvaporationManager.restingList.Add(_blockPos.x, new DictionaryList<int, DictionaryList<int, WaterEvaporationManager.BlockData>>());
          WaterEvaporationManager.restingList.dict[_blockPos.x].Add(_blockPos.y, new DictionaryList<int, WaterEvaporationManager.BlockData>());
          WaterEvaporationManager.restingList.dict[_blockPos.x].dict[_blockPos.y].Add(_blockPos.z, blockData);
          WaterEvaporationManager.restWalkList.Add(blockData.ID, blockData);
        }
      }
    }
  }

  public static void RemoveFromRestList(Vector3i _blockPos)
  {
    lock (WaterEvaporationManager.restingList)
    {
      if (!WaterEvaporationManager.restingList.dict.ContainsKey(_blockPos.x) || !WaterEvaporationManager.restingList.dict[_blockPos.x].dict.ContainsKey(_blockPos.y) || !WaterEvaporationManager.restingList.dict[_blockPos.x].dict[_blockPos.y].dict.ContainsKey(_blockPos.z))
        return;
      lock (WaterEvaporationManager.restWalkList)
        WaterEvaporationManager.restWalkList.Remove(WaterEvaporationManager.restingList.dict[_blockPos.x].dict[_blockPos.y].dict[_blockPos.z].ID);
      WaterEvaporationManager.restingList.dict[_blockPos.x].dict[_blockPos.y].Remove(_blockPos.z);
    }
  }

  public static ulong GetRestTime(Vector3i _blockPos)
  {
    lock (WaterEvaporationManager.restingList)
    {
      if (WaterEvaporationManager.restingList.dict.ContainsKey(_blockPos.x))
      {
        if (WaterEvaporationManager.restingList.dict[_blockPos.x].dict.ContainsKey(_blockPos.y))
        {
          if (WaterEvaporationManager.restingList.dict[_blockPos.x].dict[_blockPos.y].dict.ContainsKey(_blockPos.z))
            return WaterEvaporationManager.restingList.dict[_blockPos.x].dict[_blockPos.y].dict[_blockPos.z].time;
        }
      }
    }
    return 0;
  }

  public static void AddToEvapList(Vector3i _blockPos)
  {
    lock (WaterEvaporationManager.evaporationList)
    {
      lock (WaterEvaporationManager.evapWalkList)
      {
        if (WaterEvaporationManager.evaporationList.dict.ContainsKey(_blockPos.x))
        {
          if (WaterEvaporationManager.evaporationList.dict[_blockPos.x].dict.ContainsKey(_blockPos.y))
          {
            if (WaterEvaporationManager.evaporationList.dict[_blockPos.x].dict[_blockPos.y].dict.ContainsKey(_blockPos.z))
            {
              WaterEvaporationManager.BlockData blockData = new WaterEvaporationManager.BlockData(_blockPos);
              WaterEvaporationManager.evaporationList.dict[_blockPos.x].dict[_blockPos.y].dict[_blockPos.z] = blockData;
              WaterEvaporationManager.evapWalkList.Add(blockData.ID, blockData);
            }
            else
            {
              WaterEvaporationManager.BlockData blockData = new WaterEvaporationManager.BlockData(_blockPos);
              WaterEvaporationManager.evaporationList.dict[_blockPos.x].dict[_blockPos.y].Add(_blockPos.z, blockData);
              WaterEvaporationManager.evapWalkList.Add(blockData.ID, blockData);
            }
          }
          else
          {
            WaterEvaporationManager.BlockData blockData = new WaterEvaporationManager.BlockData(_blockPos);
            WaterEvaporationManager.evaporationList.dict[_blockPos.x].Add(_blockPos.y, new DictionaryList<int, WaterEvaporationManager.BlockData>());
            WaterEvaporationManager.evaporationList.dict[_blockPos.x].dict[_blockPos.y].Add(_blockPos.z, blockData);
            WaterEvaporationManager.evapWalkList.Add(blockData.ID, blockData);
          }
        }
        else
        {
          WaterEvaporationManager.BlockData blockData = new WaterEvaporationManager.BlockData(_blockPos);
          WaterEvaporationManager.evaporationList.Add(_blockPos.x, new DictionaryList<int, DictionaryList<int, WaterEvaporationManager.BlockData>>());
          WaterEvaporationManager.evaporationList.dict[_blockPos.x].Add(_blockPos.y, new DictionaryList<int, WaterEvaporationManager.BlockData>());
          WaterEvaporationManager.evaporationList.dict[_blockPos.x].dict[_blockPos.y].Add(_blockPos.z, blockData);
          WaterEvaporationManager.evapWalkList.Add(blockData.ID, blockData);
        }
      }
    }
  }

  public static void RemoveFromEvapList(Vector3i _blockPos)
  {
    lock (WaterEvaporationManager.evaporationList)
    {
      if (!WaterEvaporationManager.evaporationList.dict.ContainsKey(_blockPos.x) || !WaterEvaporationManager.evaporationList.dict[_blockPos.x].dict.ContainsKey(_blockPos.y) || !WaterEvaporationManager.evaporationList.dict[_blockPos.x].dict[_blockPos.y].dict.ContainsKey(_blockPos.z))
        return;
      lock (WaterEvaporationManager.evapWalkList)
        WaterEvaporationManager.evapWalkList.Remove(WaterEvaporationManager.evaporationList.dict[_blockPos.x].dict[_blockPos.y].dict[_blockPos.z].ID);
      WaterEvaporationManager.evaporationList.dict[_blockPos.x].dict[_blockPos.y].Remove(_blockPos.z);
    }
  }

  public static ulong GetEvapTime(Vector3i _blockPos)
  {
    lock (WaterEvaporationManager.evaporationList)
    {
      if (WaterEvaporationManager.evaporationList.dict.ContainsKey(_blockPos.x))
      {
        if (WaterEvaporationManager.evaporationList.dict[_blockPos.x].dict.ContainsKey(_blockPos.y))
        {
          if (WaterEvaporationManager.evaporationList.dict[_blockPos.x].dict[_blockPos.y].dict.ContainsKey(_blockPos.z))
            return WaterEvaporationManager.evaporationList.dict[_blockPos.x].dict[_blockPos.y].dict[_blockPos.z].time;
        }
      }
    }
    return 0;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  static WaterEvaporationManager()
  {
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public class BlockData
  {
    public ulong time;
    public ulong ID;
    public Vector3i pos;

    public BlockData(Vector3i _pos)
    {
      this.time = GameManager.Instance.World.GetWorldTime();
      this.ID = WaterEvaporationManager.uniqueIndex++;
      this.pos = _pos;
    }
  }
}
