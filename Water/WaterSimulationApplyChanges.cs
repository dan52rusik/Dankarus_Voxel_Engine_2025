// Decompiled with JetBrains decompiler
// Type: WaterSimulationApplyChanges
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using System.Collections.Generic;
using Unity.Mathematics;

#nullable disable
public class WaterSimulationApplyChanges
{
  [PublicizedFrom(EAccessModifier.Private)]
  public MemoryPooledObject<WaterSimulationApplyChanges.ChangesForChunk> changesPool = new MemoryPooledObject<WaterSimulationApplyChanges.ChangesForChunk>(300);
  [PublicizedFrom(EAccessModifier.Private)]
  public ChunkCluster chunks;
  [PublicizedFrom(EAccessModifier.Private)]
  public ThreadManager.ThreadInfo applyThread;
  [PublicizedFrom(EAccessModifier.Private)]
  public const int noWorkPauseDurationMs = 15;
  [PublicizedFrom(EAccessModifier.Private)]
  public Dictionary<long, WaterSimulationApplyChanges.ChangesForChunk> changeCache = new Dictionary<long, WaterSimulationApplyChanges.ChangesForChunk>();
  [PublicizedFrom(EAccessModifier.Private)]
  public LinkedList<long> changedChunkList = new LinkedList<long>();
  [PublicizedFrom(EAccessModifier.Private)]
  public NetPackageMeasure networkMeasure = new NetPackageMeasure(1.0);
  public long networkMaxBytesPerSecond = 524288 /*0x080000*/;
  [PublicizedFrom(EAccessModifier.Private)]
  public List<ClientInfo> clientsNearChunkBuffer = new List<ClientInfo>();
  [PublicizedFrom(EAccessModifier.Private)]
  public bool isServer;

  public WaterSimulationApplyChanges(ChunkCluster _cc)
  {
    this.chunks = _cc;
    this.isServer = SingletonMonoBehaviour<ConnectionManager>.Instance.IsServer;
    this.applyThread = ThreadManager.StartThread(nameof (WaterSimulationApplyChanges), (ThreadManager.ThreadFunctionDelegate) null, new ThreadManager.ThreadFunctionLoopDelegate(this.ThreadLoop), (ThreadManager.ThreadFunctionEndDelegate) null, _useRealThread: true);
  }

  public WaterSimulationApplyChanges.ChangesForChunk.Writer GetChangeWriter(long _chunkKey)
  {
    lock (this.changeCache)
    {
      WaterSimulationApplyChanges.ChangesForChunk _changes;
      if (!this.changeCache.TryGetValue(_chunkKey, out _changes))
      {
        _changes = this.changesPool.AllocSync(true);
        this.changeCache.Add(_chunkKey, _changes);
        this.changedChunkList.AddLast(_chunkKey);
      }
      return new WaterSimulationApplyChanges.ChangesForChunk.Writer(_changes);
    }
  }

  public void DiscardChangesForChunks(List<long> _chunkKeys)
  {
    lock (this.changeCache)
    {
      foreach (long chunkKey in _chunkKeys)
      {
        WaterSimulationApplyChanges.ChangesForChunk _t;
        if (this.changeCache.TryGetValue(chunkKey, out _t))
        {
          this.changesPool.FreeSync(_t);
          this.changeCache.Remove(chunkKey);
          this.changedChunkList.Remove(chunkKey);
          Log.Out($"[DiscardChangesForChunks] Discarding pending water changes for chunk: {chunkKey}");
        }
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public int ThreadLoop(ThreadManager.ThreadInfo _threadInfo)
  {
    if (_threadInfo.TerminationRequested())
      return -1;
    if (this.isServer && this.networkMaxBytesPerSecond > 0L)
    {
      lock (this.networkMeasure)
        this.networkMeasure.RecalculateTotals();
    }
    Chunk _chunk;
    WaterSimulationApplyChanges.ChangesForChunk _changes;
    if (!this.TryFindChangeToApply(out _chunk, out _changes))
      return 15;
    this.ApplyChanges(_chunk, _changes.changedVoxels);
    _chunk.EnterWriteLock();
    _chunk.InProgressWaterSim = false;
    _chunk.ExitWriteLock();
    this.changesPool.FreeSync(_changes);
    return 0;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public bool TryFindChangeToApply(
    out Chunk _chunk,
    out WaterSimulationApplyChanges.ChangesForChunk _changes)
  {
    lock (this.changeCache)
    {
      if (this.changedChunkList.Count == 0)
      {
        _chunk = (Chunk) null;
        _changes = (WaterSimulationApplyChanges.ChangesForChunk) null;
        return false;
      }
      LinkedListNode<long> linkedListNode = this.changedChunkList.First;
      while (linkedListNode != null)
      {
        long num = linkedListNode.Value;
        if (!this.changeCache.TryGetValue(num, out _changes))
        {
          LinkedListNode<long> node = linkedListNode;
          linkedListNode = linkedListNode.Next;
          this.changedChunkList.Remove(node);
        }
        else if (_changes.IsRecordingChanges)
          linkedListNode = linkedListNode.Next;
        else if (!WaterUtils.TryOpenChunkForUpdate(this.chunks, num, out _chunk))
        {
          linkedListNode = linkedListNode.Next;
        }
        else
        {
          LinkedListNode<long> node = linkedListNode;
          LinkedListNode<long> next = linkedListNode.Next;
          this.changedChunkList.Remove(node);
          this.changeCache.Remove(num);
          return true;
        }
      }
      _chunk = (Chunk) null;
      _changes = (WaterSimulationApplyChanges.ChangesForChunk) null;
      return false;
    }
  }

  public void ApplyChanges(Chunk _chunk, Dictionary<int, WaterValue> changedVoxels)
  {
    NetPackageWaterSimChunkUpdate _package = (NetPackageWaterSimChunkUpdate) null;
    if (this.isServer && SingletonMonoBehaviour<ConnectionManager>.Instance.ClientCount() > 0)
      _package = this.SetupForSend(_chunk);
    int _totalBytes = 0;
    int num1 = 0;
    foreach (KeyValuePair<int, WaterValue> changedVoxel in changedVoxels)
    {
      int key = changedVoxel.Key;
      int3 voxelCoords = WaterDataHandle.GetVoxelCoords(key);
      WaterValue waterValue = changedVoxel.Value;
      WaterValue _lastData;
      _chunk.SetWaterSimUpdate(voxelCoords.x, voxelCoords.y, voxelCoords.z, waterValue, out _lastData);
      if (waterValue.GetMass() != _lastData.GetMass())
      {
        if (WaterUtils.GetWaterLevel(_lastData) != WaterUtils.GetWaterLevel(waterValue))
          num1 |= 1 << voxelCoords.y / 16 /*0x10*/;
        _package?.AddChange((ushort) key, waterValue);
      }
    }
    if (_package != null)
    {
      _package.FinalizeSend();
      _totalBytes += this.SendUpdateToClients(_package);
    }
    if (num1 != 0)
    {
      lock (_chunk)
      {
        int needsRegenerationAt = _chunk.NeedsRegenerationAt;
        int num2;
        _chunk.SetNeedsRegenerationRaw(num2 = needsRegenerationAt | num1);
      }
    }
    if (_totalBytes <= 0)
      return;
    if (this.networkMaxBytesPerSecond > 0L)
    {
      lock (this.networkMeasure)
        this.networkMeasure.AddSample((long) _totalBytes);
    }
    SingletonMonoBehaviour<ConnectionManager>.Instance.FlushClientSendQueues();
  }

  public bool HasNetWorkLimitBeenReached()
  {
    if (!this.isServer || this.networkMaxBytesPerSecond <= 0L || SingletonMonoBehaviour<ConnectionManager>.Instance.ClientCount() <= 0)
      return false;
    lock (this.networkMeasure)
      return this.networkMeasure.totalSent > this.networkMaxBytesPerSecond;
  }

  public NetPackageWaterSimChunkUpdate SetupForSend(Chunk _chunk)
  {
    this.clientsNearChunkBuffer.Clear();
    long key = _chunk.Key;
    foreach (ClientInfo clientInfo in SingletonMonoBehaviour<ConnectionManager>.Instance.Clients.List)
    {
      EntityPlayer entityPlayer;
      if (GameManager.Instance.World.Players.dict.TryGetValue(clientInfo.entityId, out entityPlayer) && entityPlayer.ChunkObserver.chunksAround.Contains(key))
        this.clientsNearChunkBuffer.Add(clientInfo);
    }
    NetPackageWaterSimChunkUpdate package = NetPackageManager.GetPackage<NetPackageWaterSimChunkUpdate>();
    package.SetupForSend(_chunk);
    return package;
  }

  public int SendUpdateToClients(NetPackageWaterSimChunkUpdate _package)
  {
    int clients = 0;
    _package.RegisterSendQueue();
    foreach (ClientInfo clientInfo in this.clientsNearChunkBuffer)
    {
      clientInfo.SendPackage((NetPackage) _package);
      clients += _package.GetLength();
    }
    _package.SendQueueHandled();
    return clients;
  }

  public void Cleanup() => this.applyThread.WaitForEnd();

  public class ChangesForChunk : IMemoryPoolableObject
  {
    [PublicizedFrom(EAccessModifier.Private)]
    public bool isRecordingChanges;
    public Dictionary<int, WaterValue> changedVoxels = new Dictionary<int, WaterValue>();

    public bool IsRecordingChanges
    {
      get
      {
        lock (this)
          return this.isRecordingChanges;
      }
    }

    public void Cleanup() => this.Reset();

    public void Reset()
    {
      this.isRecordingChanges = false;
      this.changedVoxels.Clear();
    }

    public struct Writer : IDisposable
    {
      [PublicizedFrom(EAccessModifier.Private)]
      public WaterSimulationApplyChanges.ChangesForChunk changes;

      public Writer(
        WaterSimulationApplyChanges.ChangesForChunk _changes)
      {
        lock (_changes)
          _changes.isRecordingChanges = true;
        this.changes = _changes;
      }

      public void RecordChange(int _voxelIndex, WaterValue _waterValue)
      {
        this.changes.changedVoxels[_voxelIndex] = _waterValue;
      }

      public void Dispose()
      {
        lock (this.changes)
          this.changes.isRecordingChanges = false;
      }
    }
  }
}
