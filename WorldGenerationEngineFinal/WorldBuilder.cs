// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.WorldBuilder
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using InControl;
using Platform;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using UnityEngine;
using UnityEngine.Experimental.Rendering;

#nullable disable
namespace WorldGenerationEngineFinal;

public class WorldBuilder
{
  [PublicizedFrom(EAccessModifier.Private)]
  public const int groundHeight = 35;
  [PublicizedFrom(EAccessModifier.Private)]
  public const float groundHeightPer = 0.137254909f;
  public int WaterHeight = 30;
  public readonly DistrictPlanner DistrictPlanner;
  public readonly HighwayPlanner HighwayPlanner;
  public readonly PathingUtils PathingUtils;
  public readonly PathShared PathShared;
  public readonly POISmoother POISmoother;
  public readonly PrefabManager PrefabManager;
  public readonly StampManager StampManager;
  public readonly StreetTileShared StreetTileShared;
  public readonly TownPlanner TownPlanner;
  public readonly TownshipShared TownshipShared;
  public readonly WildernessPathPlanner WildernessPathPlanner;
  public readonly WildernessPlanner WildernessPlanner;
  public string WorldName;
  public string WorldSeedName;
  [PublicizedFrom(EAccessModifier.Private)]
  public string WorldPath;
  [PublicizedFrom(EAccessModifier.Private)]
  public MicroStopwatch totalMS;
  public bool IsCanceled;
  public bool IsFinished;
  [PublicizedFrom(EAccessModifier.Private)]
  public Color32[] roadDest;
  public Texture2D PreviewImage;
  public int WorldSize = 8192 /*0x2000*/;
  public int WorldSizeDistDiv;
  public const int BiomeSizeDiv = 8;
  public int BiomeSize;
  public int Seed = 12345;
  public int Plains = 4;
  public int Hills = 4;
  public int Mountains = 2;
  public WorldBuilder.GenerationSelections Canyons = WorldBuilder.GenerationSelections.Default;
  public WorldBuilder.GenerationSelections Craters = WorldBuilder.GenerationSelections.Default;
  public WorldBuilder.GenerationSelections Lakes = WorldBuilder.GenerationSelections.Default;
  public WorldBuilder.GenerationSelections Rivers = WorldBuilder.GenerationSelections.Default;
  public WorldBuilder.GenerationSelections Towns = WorldBuilder.GenerationSelections.Default;
  public WorldBuilder.GenerationSelections Wilderness = WorldBuilder.GenerationSelections.Default;
  [PublicizedFrom(EAccessModifier.Private)]
  public float[] HeightMap;
  [PublicizedFrom(EAccessModifier.Private)]
  public byte[] WaterMap;
  public byte[,] PathCreationGrid;
  public PathTile[,] PathingGrid;
  public int StreetTileMapSize;
  public StreetTile[,] StreetTileMap;
  public byte[] poiHeightMask;
  public DataMap<BiomeType> biomeMap;
  public DataMap<TerrainType> terrainTypeMap;
  public List<Township> Townships = new List<Township>();
  public int WildernessPrefabCount;
  public DynamicPrefabDecorator PrefabDecorator;
  [PublicizedFrom(EAccessModifier.Private)]
  public string worldSizeName;
  [PublicizedFrom(EAccessModifier.Private)]
  public DynamicProperties thisWorldProperties;
  [PublicizedFrom(EAccessModifier.Private)]
  public const int WorldTileSize = 1024 /*0x0400*/;
  [PublicizedFrom(EAccessModifier.Private)]
  public const int BiomeTileSize = 256 /*0x0100*/;
  [PublicizedFrom(EAccessModifier.Private)]
  public const int TerrainTileSize = 256 /*0x0100*/;
  [PublicizedFrom(EAccessModifier.Private)]
  public const int terrainToBiomeTileScale = 1;
  public WorldBuilder.BiomeLayout biomeLayout;
  public int ForestBiomeWeight = 13;
  [PublicizedFrom(EAccessModifier.Private)]
  public int BurntForestBiomeWeight = 18;
  [PublicizedFrom(EAccessModifier.Private)]
  public int DesertBiomeWeight = 22;
  [PublicizedFrom(EAccessModifier.Private)]
  public int SnowBiomeWeight = 23;
  [PublicizedFrom(EAccessModifier.Private)]
  public int WastelandBiomeWeight = 24;
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly Dictionary<BiomeType, Color32> biomeColors = new Dictionary<BiomeType, Color32>();
  [PublicizedFrom(EAccessModifier.Private)]
  public const int cPlayerSpawnsNeeded = 12;
  [PublicizedFrom(EAccessModifier.Private)]
  public List<WorldBuilder.PlayerSpawn> playerSpawns;
  public List<Path> highwayPaths = new List<Path>();
  public List<Path> wildernessPaths = new List<Path>();
  public List<Vector2i> TraderCenterPositions = new List<Vector2i>();
  public List<Vector2i> TraderForestCenterPositions = new List<Vector2i>();
  [PublicizedFrom(EAccessModifier.Private)]
  public WaitForEndOfFrame endOfFrameHandle = new WaitForEndOfFrame();
  public bool UsePreviewer = true;
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly (string langKey, string fileName, Action<Stream> serializer)[] threadedSerializers;
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly MemoryStream[] threadedSerializerBuffers;
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly (string langKey, string fileName, Func<Stream, IEnumerator> serializer)[] mainThreadSerializers;
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly MemoryStream[] mainThreadSerializerBuffers;
  [PublicizedFrom(EAccessModifier.Private)]
  public long serializedTotalSize;
  public readonly int[] biomeTagBits = new int[5]
  {
    FastTags<TagGroup.Poi>.GetBit("forest"),
    FastTags<TagGroup.Poi>.GetBit("burntforest"),
    FastTags<TagGroup.Poi>.GetBit("desert"),
    FastTags<TagGroup.Poi>.GetBit("snow"),
    FastTags<TagGroup.Poi>.GetBit("wasteland")
  };
  [PublicizedFrom(EAccessModifier.Private)]
  public float[] terrainDest;
  [PublicizedFrom(EAccessModifier.Private)]
  public float[] terrainWaterDest;
  [PublicizedFrom(EAccessModifier.Private)]
  public float[] waterDest;
  [PublicizedFrom(EAccessModifier.Private)]
  public Color32[] biomeDest;
  [PublicizedFrom(EAccessModifier.Private)]
  public Color32[] radDest;
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly StampGroup lowerLayer = new StampGroup("Lower Layer");
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly StampGroup terrainLayer = new StampGroup("Top Layer");
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly StampGroup radiationLayer = new StampGroup("Radiation Layer");
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly StampGroup biomeLayer = new StampGroup("Biome Layer");
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly StampGroup waterLayer = new StampGroup("Water Layer");
  [PublicizedFrom(EAccessModifier.Private)]
  public bool GenWaterBorderN;
  [PublicizedFrom(EAccessModifier.Private)]
  public bool GenWaterBorderS;
  [PublicizedFrom(EAccessModifier.Private)]
  public bool GenWaterBorderW;
  [PublicizedFrom(EAccessModifier.Private)]
  public bool GenWaterBorderE;
  public List<Rect> waterRects = new List<Rect>();
  public List<WorldBuilder.WildernessPathInfo> wPathInfo = new List<WorldBuilder.WildernessPathInfo>();
  [PublicizedFrom(EAccessModifier.Private)]
  public string setMessageLast = string.Empty;
  [PublicizedFrom(EAccessModifier.Private)]
  public MicroStopwatch messageMS = new MicroStopwatch(true);
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly Vector2i[] directions8way = new Vector2i[8]
  {
    Vector2i.up,
    Vector2i.up + Vector2i.right,
    Vector2i.right,
    Vector2i.right + Vector2i.down,
    Vector2i.down,
    Vector2i.down + Vector2i.left,
    Vector2i.left,
    Vector2i.left + Vector2i.up
  };
  [PublicizedFrom(EAccessModifier.Private)]
  public readonly Vector2i[] directions4way = new Vector2i[4]
  {
    Vector2i.up,
    Vector2i.right,
    Vector2i.down,
    Vector2i.left
  };

  public int HalfWorldSize => this.WorldSize / 2;

  public Vector3i PrefabWorldOffset => new Vector3i(-this.HalfWorldSize, 0, -this.HalfWorldSize);

  public long SerializedSize => !SaveInfoProvider.DataLimitEnabled ? 0L : this.serializedTotalSize;

  public WorldBuilder(string _seed, int _worldSize)
  {
    this.WorldSeedName = _seed;
    this.WorldSize = _worldSize;
    this.WorldName = WorldBuilder.GetGeneratedWorldName(this.WorldSeedName, this.WorldSize);
    this.WorldPath = $"{GameIO.GetUserGameDataDir()}/GeneratedWorlds/{this.WorldName}/";
    this.WorldSizeDistDiv = this.WorldSize > 4500 ? 1 : (this.WorldSize > 3500 ? 2 : (this.WorldSize > 2500 ? 3 : 4));
    this.DistrictPlanner = new DistrictPlanner(this);
    this.HighwayPlanner = new HighwayPlanner(this);
    this.PathingUtils = new PathingUtils(this);
    this.PathShared = new PathShared(this);
    this.POISmoother = new POISmoother(this);
    this.PrefabManager = new PrefabManager(this);
    this.StampManager = new StampManager(this);
    this.StreetTileShared = new StreetTileShared(this);
    this.TownPlanner = new TownPlanner(this);
    this.TownshipShared = new TownshipShared(this);
    this.WildernessPathPlanner = new WildernessPathPlanner(this);
    this.WildernessPlanner = new WildernessPlanner(this);
    List<(string, string, Action<Stream>)> valueTupleList1 = new List<(string, string, Action<Stream>)>();
    List<(string, string, Func<Stream, IEnumerator>)> valueTupleList2 = new List<(string, string, Func<Stream, IEnumerator>)>();
    valueTupleList1.Add(("xuiBiomes", "biomes.png", (Action<Stream>) ([PublicizedFrom(EAccessModifier.Private)] (stream) => stream.Write(ReadOnlySpan<byte>.op_Implicit(ImageConversion.EncodeArrayToPNG((Array) this.biomeDest, GraphicsFormat.R8G8B8A8_UNorm, (uint) this.BiomeSize, (uint) this.BiomeSize, (uint) (this.BiomeSize * 4)))))));
    valueTupleList1.Add(("xuiRadiation", "radiation.png", (Action<Stream>) ([PublicizedFrom(EAccessModifier.Private)] (stream) => stream.Write(ReadOnlySpan<byte>.op_Implicit(ImageConversion.EncodeArrayToPNG((Array) this.radDest, GraphicsFormat.R8G8B8A8_UNorm, (uint) this.WorldSize, (uint) this.WorldSize, (uint) (this.WorldSize * 4)))))));
    valueTupleList1.Add(("xuiRoads", "splat3.png", (Action<Stream>) ([PublicizedFrom(EAccessModifier.Private)] (stream) => stream.Write(ReadOnlySpan<byte>.op_Implicit(ImageConversion.EncodeArrayToPNG((Array) this.roadDest, GraphicsFormat.R8G8B8A8_UNorm, (uint) this.WorldSize, (uint) this.WorldSize, (uint) (this.WorldSize * 4)))))));
    valueTupleList1.Add(("xuiWater", "splat4.png", new Action<Stream>(this.serializeWater)));
    valueTupleList1.Add(("xuiHeightmap", "dtm.raw", new Action<Stream>(this.serializeRawHeightmap)));
    valueTupleList1.Add(("xuiPrefabs", "prefabs.xml", new Action<Stream>(this.serializePrefabs)));
    valueTupleList1.Add(("xuiPlayerSpawns", "spawnpoints.xml", new Action<Stream>(this.serializePlayerSpawns)));
    valueTupleList1.Add(("xuiLevelMetadata", "main.ttw", new Action<Stream>(this.serializeRWGTTW)));
    valueTupleList1.Add(("xuiMapInfo", "map_info.xml", new Action<Stream>(this.serializeDynamicProperties)));
    this.threadedSerializers = valueTupleList1.ToArray();
    this.mainThreadSerializers = valueTupleList2.ToArray();
    this.threadedSerializerBuffers = new MemoryStream[this.threadedSerializers.Length];
    this.mainThreadSerializerBuffers = new MemoryStream[this.mainThreadSerializers.Length];
  }

  public WorldBuilder(int _worldSize)
  {
    this.WorldSize = _worldSize;
    this.PathingUtils = new PathingUtils(this);
    int length = this.WorldSize * this.WorldSize;
    this.HeightMap = new float[length];
    this.WaterMap = new byte[length];
    this.radDest = new Color32[length];
    this.waterDest = new float[length];
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public IEnumerator Init()
  {
    if (PlatformOptimizations.RestartAfterRwg)
      PlatformApplicationManager.SetRestartRequired();
    this.roadDest = new Color32[this.WorldSize * this.WorldSize];
    yield return (object) this.StampManager.LoadStamps();
    this.PrefabManager.PrefabInstanceId = 0;
    this.playerSpawns = new List<WorldBuilder.PlayerSpawn>();
    this.PathingGrid = new PathTile[this.WorldSize / 10, this.WorldSize / 10];
    foreach (KeyValuePair<string, Vector2i> keyValuePair in WorldBuilderStatic.WorldSizeMapper)
    {
      string str1;
      Vector2i vector2i1;
      keyValuePair.Deconstruct(ref str1, ref vector2i1);
      string str2 = str1;
      Vector2i vector2i2 = vector2i1;
      if (this.WorldSize >= vector2i2.x && this.WorldSize < vector2i2.y)
        this.worldSizeName = str2;
    }
    if (this.worldSizeName == null)
    {
      Log.Error($"There was an error finding rwgmixer world entry for the current world size! WorldSize: {this.WorldSize}/n Please make sure that the world size falls within the min/max ranges listed in xml.");
    }
    else
    {
      this.thisWorldProperties = WorldBuilderStatic.Properties[this.worldSizeName];
      this.Seed = this.WorldSeedName.GetHashCode() + this.WorldSize;
      Rand.Instance.SetSeed(this.Seed);
      this.biomeColors[BiomeType.forest] = WorldBuilderConstants.forestCol;
      this.biomeColors[BiomeType.burntForest] = WorldBuilderConstants.burntForestCol;
      this.biomeColors[BiomeType.desert] = WorldBuilderConstants.desertCol;
      this.biomeColors[BiomeType.snow] = WorldBuilderConstants.snowCol;
      this.biomeColors[BiomeType.wasteland] = WorldBuilderConstants.wastelandCol;
    }
  }

  public void SetBiomeWeight(BiomeType _type, int _weight)
  {
    switch (_type)
    {
      case BiomeType.forest:
        this.ForestBiomeWeight = _weight;
        break;
      case BiomeType.burntForest:
        this.BurntForestBiomeWeight = _weight;
        break;
      case BiomeType.desert:
        this.DesertBiomeWeight = _weight;
        break;
      case BiomeType.snow:
        this.SnowBiomeWeight = _weight;
        break;
      case BiomeType.wasteland:
        this.WastelandBiomeWeight = _weight;
        break;
    }
  }

  public IEnumerator GenerateFromServer()
  {
    this.UsePreviewer = false;
    this.totalMS = new MicroStopwatch(true);
    yield return (object) this.GenerateData();
    yield return (object) this.SaveData(false);
    this.Cleanup();
    this.SetMessage((string) null);
  }

  public IEnumerator GenerateFromUI()
  {
    this.IsCanceled = false;
    this.IsFinished = false;
    this.totalMS = new MicroStopwatch(true);
    yield return (object) this.SetMessage(Localization.Get("xuiStarting"));
    yield return (object) new WaitForSeconds(0.1f);
    yield return (object) this.GenerateData();
  }

  public IEnumerator FinishForPreview()
  {
    MicroStopwatch ms = new MicroStopwatch(true);
    yield return (object) this.CreatePreviewTexture(this.roadDest);
    Log.Out("CreatePreviewTexture in {0}", (object) (float) ((double) ms.ElapsedMilliseconds * (1.0 / 1000.0)));
    if (!this.IsCanceled)
      yield return (object) this.SetMessage(Localization.Get("xuiRwgGenerationComplete"), true);
    else
      yield return (object) this.SetMessage(Localization.Get("xuiRwgGenerationCanceled"), true, true);
    this.IsFinished = true;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public IEnumerator GenerateData()
  {
    yield return (object) this.Init();
    yield return (object) this.SetMessage(string.Format(Localization.Get("xuiWorldGenerationGenerating"), (object) this.WorldName), true);
    yield return (object) this.generateTerrain();
    if (!this.IsCanceled)
    {
      this.initStreetTiles();
      if (!this.IsCanceled)
      {
        bool hasPOIs = this.Towns != WorldBuilder.GenerationSelections.None || this.Wilderness != 0;
        if (hasPOIs)
        {
          yield return (object) this.PrefabManager.LoadPrefabs();
          this.PrefabManager.ShufflePrefabData(this.Seed);
          yield return (object) null;
          this.PathingUtils.SetupPathingGrid();
        }
        else
          this.PrefabManager.ClearDisplayed();
        if (this.Towns != WorldBuilder.GenerationSelections.None)
          yield return (object) this.TownPlanner.Plan(this.thisWorldProperties, this.Seed);
        yield return (object) this.GenerateTerrainLast();
        if (!this.IsCanceled)
        {
          yield return (object) this.POISmoother.SmoothStreetTiles();
          if (!this.IsCanceled)
          {
            if (this.Wilderness != WorldBuilder.GenerationSelections.None)
            {
              yield return (object) this.WildernessPlanner.Plan(this.thisWorldProperties, this.Seed);
              yield return (object) this.SmoothWildernessTerrain();
              if (this.IsCanceled)
                yield break;
            }
            if (hasPOIs)
            {
              this.CalcTownshipsHeightMask();
              yield return (object) this.HighwayPlanner.Plan(this.thisWorldProperties, this.Seed);
              yield return (object) this.TownPlanner.SpawnPrefabs();
              if (this.IsCanceled)
                yield break;
            }
            if (this.Wilderness != WorldBuilder.GenerationSelections.None)
              yield return (object) this.WildernessPathPlanner.Plan(this.Seed);
            int num = 12 - this.playerSpawns.Count;
            if (num > 0)
            {
              foreach (StreetTile calcPlayerSpawnTile in this.CalcPlayerSpawnTiles())
              {
                if (this.CreatePlayerSpawn(calcPlayerSpawnTile.WorldPositionCenter, true))
                {
                  if (--num <= 0)
                    break;
                }
              }
            }
            yield return (object) GCUtils.UnloadAndCollectCo();
            yield return (object) this.SetMessage(Localization.Get("xuiRwgDrawRoads"), true);
            yield return (object) this.DrawRoads(this.roadDest);
            if (hasPOIs)
            {
              yield return (object) this.SetMessage(Localization.Get("xuiRwgSmoothRoadTerrain"), true);
              this.CalcWindernessPOIsHeightMask(this.roadDest);
              yield return (object) this.SmoothRoadTerrain(this.roadDest, this.HeightMap, this.WorldSize, this.Townships);
            }
            this.highwayPaths.Clear();
            this.wildernessPaths.Clear();
            yield return (object) this.FinalizeWater();
            yield return (object) this.SerializeData();
            yield return (object) GCUtils.UnloadAndCollectCo();
            object[] objArray = new object[3];
            TimeSpan elapsed = this.totalMS.Elapsed;
            objArray[0] = (object) elapsed.Minutes;
            elapsed = this.totalMS.Elapsed;
            objArray[1] = (object) elapsed.Seconds;
            objArray[2] = (object) Rand.Instance.PeekSample();
            Log.Out("RWG final in {0}:{1:00}, r={2:x}", objArray);
          }
        }
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public IEnumerator SerializeData()
  {
    // ISSUE: object of a compiler-generated type is created
    // ISSUE: variable of a compiler-generated type
    WorldBuilder.\u003C\u003Ec__DisplayClass91_0 cDisplayClass910 = new WorldBuilder.\u003C\u003Ec__DisplayClass91_0();
    if (SaveInfoProvider.DataLimitEnabled)
    {
      MicroStopwatch totalMs = new MicroStopwatch(true);
      // ISSUE: reference to a compiler-generated field
      cDisplayClass910.threadedSerializerTasks = new System.Threading.Tasks.Task[this.threadedSerializers.Length];
      for (int index = 0; index < this.threadedSerializers.Length; ++index)
      {
        // ISSUE: object of a compiler-generated type is created
        // ISSUE: variable of a compiler-generated type
        WorldBuilder.\u003C\u003Ec__DisplayClass91_1 cDisplayClass911 = new WorldBuilder.\u003C\u003Ec__DisplayClass91_1();
        // ISSUE: reference to a compiler-generated field
        // ISSUE: reference to a compiler-generated field
        (_, cDisplayClass911.fileName, cDisplayClass911.serializer) = this.threadedSerializers[index];
        // ISSUE: reference to a compiler-generated field
        cDisplayClass911.buffer = new MemoryStream();
        // ISSUE: reference to a compiler-generated field
        this.threadedSerializerBuffers[index] = cDisplayClass911.buffer;
        // ISSUE: reference to a compiler-generated method
        System.Threading.Tasks.Task task = new System.Threading.Tasks.Task(new System.Action(cDisplayClass911.\u003CSerializeData\u003Eg__SerializeToBuffer\u007C1));
        // ISSUE: reference to a compiler-generated field
        cDisplayClass910.threadedSerializerTasks[index] = task;
        task.Start();
      }
      for (int i = 0; i < this.mainThreadSerializers.Length; ++i)
      {
        // ISSUE: object of a compiler-generated type is created
        // ISSUE: variable of a compiler-generated type
        WorldBuilder.\u003C\u003Ec__DisplayClass91_2 cDisplayClass912 = new WorldBuilder.\u003C\u003Ec__DisplayClass91_2();
        string str;
        // ISSUE: reference to a compiler-generated field
        // ISSUE: reference to a compiler-generated field
        (str, cDisplayClass912.fileName, cDisplayClass912.serializer) = this.mainThreadSerializers[i];
        // ISSUE: reference to a compiler-generated field
        cDisplayClass912.buffer = new MemoryStream();
        // ISSUE: reference to a compiler-generated field
        this.mainThreadSerializerBuffers[i] = cDisplayClass912.buffer;
        yield return (object) this.SetMessage(string.Format(Localization.Get("xuiRwgSerializing"), (object) Localization.Get(str)));
        // ISSUE: reference to a compiler-generated method
        // ISSUE: reference to a compiler-generated method
        yield return (object) ThreadManager.CoroutineWrapperWithExceptionCallback(cDisplayClass912.\u003CSerializeData\u003Eg__SerializeToBuffer\u007C3(), new Action<Exception>(cDisplayClass912.\u003CSerializeData\u003Eb__2));
      }
      object[] lastTaskNames = (object[]) null;
      // ISSUE: reference to a compiler-generated field
      while (((IEnumerable<System.Threading.Tasks.Task>) cDisplayClass910.threadedSerializerTasks).Any<System.Threading.Tasks.Task>((Func<System.Threading.Tasks.Task, bool>) ([PublicizedFrom(EAccessModifier.Internal)] (x) => !x.IsCompleted)))
      {
        // ISSUE: reference to a compiler-generated field
        // ISSUE: reference to a compiler-generated field
        // ISSUE: reference to a compiler-generated method
        object[] array = ((IEnumerable<(string, string, Action<Stream>)>) this.threadedSerializers).Where<(string, string, Action<Stream>)>(cDisplayClass910.\u003C\u003E9__4 ?? (cDisplayClass910.\u003C\u003E9__4 = new Func<(string, string, Action<Stream>), int, bool>(cDisplayClass910.\u003CSerializeData\u003Eb__4))).Take<(string, string, Action<Stream>)>(3).Select<(string, string, Action<Stream>), string>((Func<(string, string, Action<Stream>), string>) ([PublicizedFrom(EAccessModifier.Internal)] (x) => Localization.Get(x.langKey))).Cast<object>().ToArray<object>();
        if (lastTaskNames != null && ((IEnumerable<object>) array).SequenceEqual<object>((IEnumerable<object>) lastTaskNames))
        {
          yield return (object) null;
        }
        else
        {
          lastTaskNames = array;
          yield return (object) this.SetMessage(string.Format(Localization.Get("xuiRwgSerializing"), (object) Localization.FormatListAnd(array)));
        }
      }
      long num = 0;
      foreach (MemoryStream serializerBuffer in this.threadedSerializerBuffers)
        num += serializerBuffer.Length;
      foreach (MemoryStream serializerBuffer in this.mainThreadSerializerBuffers)
        num += serializerBuffer.Length;
      this.serializedTotalSize = num;
      Log.Out($"RWG SerializeData {this.serializedTotalSize.FormatSize(true)} in {totalMs.Elapsed.TotalSeconds:F3} s");
    }
  }

  public bool CanSaveData() => !SdDirectory.Exists(this.WorldPath);

  public IEnumerator SaveData(
    bool canPrompt,
    XUiController parentController = null,
    bool autoConfirm = false,
    System.Action onCancel = null,
    System.Action onDiscard = null,
    System.Action onConfirm = null)
  {
    // ISSUE: object of a compiler-generated type is created
    // ISSUE: variable of a compiler-generated type
    WorldBuilder.\u003C\u003Ec__DisplayClass93_0 cDisplayClass930 = new WorldBuilder.\u003C\u003Ec__DisplayClass93_0();
    // ISSUE: reference to a compiler-generated field
    cDisplayClass930.\u003C\u003E4__this = this;
    if (!this.CanSaveData())
    {
      if (canPrompt)
      {
        if (onCancel != null)
        {
          onCancel();
        }
        else
        {
          System.Action action = onDiscard;
          if (action != null)
            action();
        }
      }
    }
    else
    {
      if (canPrompt)
      {
        XUiC_SaveSpaceNeeded confirmationWindow = XUiC_SaveSpaceNeeded.Open(this.SerializedSize, this.WorldPath, parentController, autoConfirm, onCancel != null, onDiscard != null, body: "xuiRwgSaveWorld", confirm: "xuiSave");
        while (confirmationWindow.IsOpen)
          yield return (object) null;
        switch (confirmationWindow.Result)
        {
          case XUiC_SaveSpaceNeeded.ConfirmationResult.Pending:
            Log.Error("Should not be pending.");
            yield break;
          case XUiC_SaveSpaceNeeded.ConfirmationResult.Cancelled:
            System.Action action1 = onCancel;
            if (action1 == null)
              yield break;
            action1();
            yield break;
          case XUiC_SaveSpaceNeeded.ConfirmationResult.Discarded:
            System.Action action2 = onDiscard;
            if (action2 == null)
              yield break;
            action2();
            yield break;
          case XUiC_SaveSpaceNeeded.ConfirmationResult.Confirmed:
            System.Action action3 = onConfirm;
            if (action3 != null)
              action3();
            confirmationWindow = (XUiC_SaveSpaceNeeded) null;
            break;
          default:
            throw new ArgumentOutOfRangeException();
        }
      }
      this.totalMS.ResetAndRestart();
      SdDirectory.CreateDirectory(this.WorldPath);
      // ISSUE: reference to a compiler-generated field
      cDisplayClass930.threadedSaveTasks = new System.Threading.Tasks.Task[this.threadedSerializers.Length];
      for (int index = 0; index < this.threadedSerializers.Length; ++index)
      {
        // ISSUE: object of a compiler-generated type is created
        // ISSUE: variable of a compiler-generated type
        WorldBuilder.\u003C\u003Ec__DisplayClass93_1 cDisplayClass931 = new WorldBuilder.\u003C\u003Ec__DisplayClass93_1();
        // ISSUE: reference to a compiler-generated field
        cDisplayClass931.CS\u0024\u003C\u003E8__locals1 = cDisplayClass930;
        // ISSUE: reference to a compiler-generated field
        cDisplayClass931.buffer = this.threadedSerializerBuffers[index];
        // ISSUE: reference to a compiler-generated field
        // ISSUE: reference to a compiler-generated field
        (_, cDisplayClass931.fileName, cDisplayClass931.serializer) = this.threadedSerializers[index];
        // ISSUE: reference to a compiler-generated method
        System.Threading.Tasks.Task task = new System.Threading.Tasks.Task(new System.Action(cDisplayClass931.\u003CSaveData\u003Eg__SaveToFile\u007C1));
        // ISSUE: reference to a compiler-generated field
        // ISSUE: reference to a compiler-generated field
        cDisplayClass931.CS\u0024\u003C\u003E8__locals1.threadedSaveTasks[index] = task;
        task.Start();
      }
      for (int i = 0; i < this.mainThreadSerializers.Length; ++i)
      {
        // ISSUE: object of a compiler-generated type is created
        // ISSUE: variable of a compiler-generated type
        WorldBuilder.\u003C\u003Ec__DisplayClass93_2 cDisplayClass932 = new WorldBuilder.\u003C\u003Ec__DisplayClass93_2();
        // ISSUE: reference to a compiler-generated field
        cDisplayClass932.CS\u0024\u003C\u003E8__locals2 = cDisplayClass930;
        // ISSUE: reference to a compiler-generated field
        cDisplayClass932.buffer = this.mainThreadSerializerBuffers[i];
        string str;
        // ISSUE: reference to a compiler-generated field
        // ISSUE: reference to a compiler-generated field
        (str, cDisplayClass932.fileName, cDisplayClass932.serializer) = this.mainThreadSerializers[i];
        yield return (object) this.SetMessage(string.Format(Localization.Get("xuiRwgSaving"), (object) Localization.Get(str)));
        // ISSUE: reference to a compiler-generated method
        // ISSUE: reference to a compiler-generated method
        yield return (object) ThreadManager.CoroutineWrapperWithExceptionCallback(cDisplayClass932.\u003CSaveData\u003Eg__SerializeToFile\u007C3(), new Action<Exception>(cDisplayClass932.\u003CSaveData\u003Eb__2));
      }
      object[] lastTaskNames = (object[]) null;
      // ISSUE: reference to a compiler-generated field
      while (((IEnumerable<System.Threading.Tasks.Task>) cDisplayClass930.threadedSaveTasks).Any<System.Threading.Tasks.Task>((Func<System.Threading.Tasks.Task, bool>) ([PublicizedFrom(EAccessModifier.Internal)] (x) => !x.IsCompleted)))
      {
        // ISSUE: reference to a compiler-generated field
        // ISSUE: reference to a compiler-generated field
        // ISSUE: reference to a compiler-generated method
        object[] array = ((IEnumerable<(string, string, Action<Stream>)>) this.threadedSerializers).Where<(string, string, Action<Stream>)>(cDisplayClass930.\u003C\u003E9__4 ?? (cDisplayClass930.\u003C\u003E9__4 = new Func<(string, string, Action<Stream>), int, bool>(cDisplayClass930.\u003CSaveData\u003Eb__4))).Take<(string, string, Action<Stream>)>(3).Select<(string, string, Action<Stream>), string>((Func<(string, string, Action<Stream>), string>) ([PublicizedFrom(EAccessModifier.Internal)] (x) => Localization.Get(x.langKey))).Cast<object>().ToArray<object>();
        if (lastTaskNames != null && ((IEnumerable<object>) array).SequenceEqual<object>((IEnumerable<object>) lastTaskNames))
        {
          yield return (object) null;
        }
        else
        {
          lastTaskNames = array;
          yield return (object) this.SetMessage(string.Format(Localization.Get("xuiRwgSaving"), (object) Localization.FormatListAnd(array)));
        }
      }
      yield return (object) this.SetMessage(Localization.Get("xuiDmCommitting"));
      yield return (object) SaveDataUtils.SaveDataManager.CommitCoroutine();
      SaveInfoProvider.Instance.ClearResources();
      yield return (object) this.SetMessage((string) null);
      Log.Out($"RWG SaveData in {this.totalMS.Elapsed.TotalSeconds:F3} s");
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public List<StreetTile> CalcPlayerSpawnTiles()
  {
    List<StreetTile> list = ((IEnumerable) this.StreetTileMap).Cast<StreetTile>().Where<StreetTile>((Func<StreetTile, bool>) ([PublicizedFrom(EAccessModifier.Private)] (st) => !st.OverlapsRadiation && !st.AllIsWater && st.Township == null && (st.District == null || st.District.name == "wilderness") && (this.ForestBiomeWeight == 0 || st.BiomeType == BiomeType.forest) && !st.Used)).ToList<StreetTile>();
    list.Sort((Comparison<StreetTile>) ([PublicizedFrom(EAccessModifier.Private)] (_t1, _t2) => this.CalcClosestTraderDistance(_t1).CompareTo(this.CalcClosestTraderDistance(_t2))));
    return list;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public float CalcClosestTraderDistance(StreetTile _st)
  {
    float num1 = float.MaxValue;
    foreach (Vector2i traderCenterPosition in this.TraderCenterPositions)
    {
      float num2 = Vector2i.Distance(_st.WorldPositionCenter, traderCenterPosition);
      if ((double) num2 < (double) num1)
        num1 = num2;
    }
    return num1;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public List<StreetTile> getWildernessTilesToSmooth()
  {
    return ((IEnumerable) this.StreetTileMap).Cast<StreetTile>().Where<StreetTile>((Func<StreetTile, bool>) ([PublicizedFrom(EAccessModifier.Internal)] (st) => st.NeedsWildernessSmoothing)).ToList<StreetTile>();
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void initStreetTiles()
  {
    this.StreetTileMapSize = this.WorldSize / 150;
    this.StreetTileMap = new StreetTile[this.StreetTileMapSize, this.StreetTileMapSize];
    for (int _x = 0; _x < this.StreetTileMapSize; ++_x)
    {
      for (int _y = 0; _y < this.StreetTileMapSize; ++_y)
        this.StreetTileMap[_x, _y] = new StreetTile(this, new Vector2i(_x, _y));
    }
  }

  public void CleanupGeneratedData()
  {
    this.roadDest = (Color32[]) null;
    this.WaterMap = (byte[]) null;
    this.terrainDest = (float[]) null;
    this.terrainWaterDest = (float[]) null;
    this.waterDest = (float[]) null;
    this.biomeDest = (Color32[]) null;
    this.radDest = (Color32[]) null;
    this.Townships.Clear();
    if (this.PathingGrid != null)
    {
      Array.Clear((Array) this.PathingGrid, 0, this.PathingGrid.Length);
      this.PathingGrid = (PathTile[,]) null;
    }
    this.PrefabManager.Clear();
    this.PathingUtils.Cleanup();
    Rand.Instance.Cleanup();
  }

  public void Cleanup()
  {
    this.serializedTotalSize = 0L;
    Span<MemoryStream> span1 = MemoryExtensions.AsSpan<MemoryStream>(this.mainThreadSerializerBuffers);
    for (int index = 0; index < span1.Length; ++index)
    {
      ref MemoryStream local = ref span1[index];
      local?.Dispose();
      local = (MemoryStream) null;
    }
    Span<MemoryStream> span2 = MemoryExtensions.AsSpan<MemoryStream>(this.threadedSerializerBuffers);
    for (int index = 0; index < span2.Length; ++index)
    {
      ref MemoryStream local = ref span2[index];
      local?.Dispose();
      local = (MemoryStream) null;
    }
    this.CleanupGeneratedData();
    this.PrefabManager.Cleanup();
    this.StampManager.ClearStamps();
    this.HeightMap = (float[]) null;
    if ((bool) (UnityEngine.Object) this.PreviewImage)
      UnityEngine.Object.Destroy((UnityEngine.Object) this.PreviewImage);
    GCUtils.UnloadAndCollectStart();
    this.IsFinished = true;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public IEnumerator generateTerrain()
  {
    int length = this.WorldSize * this.WorldSize;
    this.HeightMap = new float[length];
    this.WaterMap = new byte[length];
    this.terrainDest = new float[length];
    this.terrainWaterDest = new float[length];
    this.BiomeSize = this.WorldSize / 8;
    this.biomeDest = new Color32[this.BiomeSize * this.BiomeSize];
    this.radDest = new Color32[length];
    this.waterDest = new float[length];
    this.GenWaterBorderN = (double) Rand.Instance.Float() > 0.5;
    this.GenWaterBorderS = (double) Rand.Instance.Float() > 0.5;
    this.GenWaterBorderW = (double) Rand.Instance.Float() > 0.5;
    this.GenWaterBorderE = (double) Rand.Instance.Float() > 0.5;
    Log.Out("generateBiomeTiles start at {0}, r={1:x}", (object) (float) ((double) this.totalMS.ElapsedMilliseconds * (1.0 / 1000.0)), (object) Rand.Instance.PeekSample());
    this.GenerateBiomeTiles();
    yield return (object) null;
    if (!this.IsCanceled)
    {
      Log.Out("GenerateTerrainTiles start at {0}, r={1:x}", (object) (float) ((double) this.totalMS.ElapsedMilliseconds * (1.0 / 1000.0)), (object) Rand.Instance.PeekSample());
      this.GenerateTerrainTiles();
      yield return (object) null;
      if (!this.IsCanceled)
      {
        Log.Out("generateBaseStamps start at {0}, r={1:x}", (object) (float) ((double) this.totalMS.ElapsedMilliseconds * (1.0 / 1000.0)), (object) Rand.Instance.PeekSample());
        yield return (object) this.generateBaseStamps();
        if (!this.IsCanceled)
        {
          yield return (object) this.GenerateTerrainFromTiles(TerrainType.plains, 1024 /*0x0400*/);
          if (!this.IsCanceled)
          {
            yield return (object) this.GenerateTerrainFromTiles(TerrainType.hills, 512 /*0x0200*/);
            if (!this.IsCanceled)
            {
              yield return (object) this.GenerateTerrainFromTiles(TerrainType.mountains, 256 /*0x0100*/);
              if (!this.IsCanceled)
              {
                Log.Out("writeStampsToMaps start at {0}, r={1:x}", (object) (float) ((double) this.totalMS.ElapsedMilliseconds * (1.0 / 1000.0)), (object) Rand.Instance.PeekSample());
                yield return (object) this.writeStampsToMaps();
                yield return (object) this.SetMessage(Localization.Get("xuiRwgTerrainGenerationFinished"));
              }
            }
          }
        }
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public IEnumerator generateBaseStamps()
  {
    this.StampManager.GetStamp("ground");
    for (int index = 0; index < this.terrainDest.Length; ++index)
    {
      this.terrainDest[index] = 0.137254909f;
      this.terrainWaterDest[index] = 0.137254909f;
    }
    Vector2 sizeMinMax = new Vector2(1.5f, 3.5f);
    this.thisWorldProperties.ParseVec("border.scale", ref sizeMinMax);
    int borderStep = 512 /*0x0200*/;
    this.thisWorldProperties.ParseInt("border_step_distance", ref borderStep);
    System.Threading.Tasks.Task terrainBorderTask = new System.Threading.Tasks.Task((System.Action) ([PublicizedFrom(EAccessModifier.Internal)] () =>
    {
      MicroStopwatch microStopwatch1 = new MicroStopwatch(true);
      MicroStopwatch microStopwatch2 = new MicroStopwatch(false);
      MicroStopwatch microStopwatch3 = new MicroStopwatch(false);
      Rand _rand = new Rand(this.Seed + 1);
      int num = borderStep;
      int max = num / 2;
      for (int index1 = 0; index1 < this.WorldSize + num && !this.IsCanceled; index1 += num)
      {
        if (!this.GenWaterBorderE || !this.GenWaterBorderW || !this.GenWaterBorderN || !this.GenWaterBorderS)
        {
          for (int index2 = 0; index2 < 4; ++index2)
          {
            TranslationData _transData = (TranslationData) null;
            if (index2 == 0 && !this.GenWaterBorderS)
              _transData = new TranslationData(index1 + _rand.Range(0, max), _rand.Range(0, max), _rand.Range(sizeMinMax.x, sizeMinMax.y), _rand.Angle());
            else if (index2 == 1 && !this.GenWaterBorderN)
              _transData = new TranslationData(index1 + _rand.Range(0, max), this.WorldSize - _rand.Range(0, max), _rand.Range(sizeMinMax.x, sizeMinMax.y), _rand.Angle());
            else if (index2 == 2 && !this.GenWaterBorderW)
              _transData = new TranslationData(_rand.Range(0, max), index1 + _rand.Range(0, max), _rand.Range(sizeMinMax.x, sizeMinMax.y), _rand.Angle());
            else if (index2 == 3 && !this.GenWaterBorderE)
              _transData = new TranslationData(this.WorldSize - _rand.Range(0, max), index1 + _rand.Range(0, max), _rand.Range(sizeMinMax.x, sizeMinMax.y), _rand.Angle());
            RawStamp _output;
            if (_transData != null && (this.StampManager.TryGetStamp(this.biomeMap.data[Mathf.Clamp(_transData.x / 1024 /*0x0400*/, 0, this.WorldSize / 1024 /*0x0400*/ - 1), Mathf.Clamp(_transData.y / 1024 /*0x0400*/, 0, this.WorldSize / 1024 /*0x0400*/ - 1)].ToString() + "_land_border", out _output, _rand) || this.StampManager.TryGetStamp("land_border", out _output, _rand)))
              this.StampManager.DrawStamp(this.terrainDest, this.terrainWaterDest, new Stamp(this, _output, _transData));
          }
        }
        if (this.GenWaterBorderE || this.GenWaterBorderW || this.GenWaterBorderN || this.GenWaterBorderS)
        {
          for (int index3 = 0; index3 < 4; ++index3)
          {
            TranslationData _transData = (TranslationData) null;
            if (index3 == 0 && this.GenWaterBorderS)
              _transData = new TranslationData(index1, _rand.Range(0, max / 2), _rand.Range(sizeMinMax.x, sizeMinMax.y), _rand.Angle());
            else if (index3 == 1 && this.GenWaterBorderN)
              _transData = new TranslationData(index1, this.WorldSize - _rand.Range(0, max / 2), _rand.Range(sizeMinMax.x, sizeMinMax.y), _rand.Angle());
            else if (index3 == 2 && this.GenWaterBorderW)
              _transData = new TranslationData(_rand.Range(0, max / 2), index1, _rand.Range(sizeMinMax.x, sizeMinMax.y), _rand.Angle());
            else if (index3 == 3 && this.GenWaterBorderE)
              _transData = new TranslationData(this.WorldSize - _rand.Range(0, max / 2), index1, _rand.Range(sizeMinMax.x, sizeMinMax.y), _rand.Angle());
            RawStamp _output;
            if (_transData != null && (this.StampManager.TryGetStamp(this.biomeMap.data[Mathf.Clamp(_transData.x / 1024 /*0x0400*/, 0, this.WorldSize / 1024 /*0x0400*/ - 1), Mathf.Clamp(_transData.y / 1024 /*0x0400*/, 0, this.WorldSize / 1024 /*0x0400*/ - 1)].ToString() + "_water_border", out _output, _rand) || this.StampManager.TryGetStamp("water_border", out _output, _rand)))
            {
              this.StampManager.DrawStamp(this.terrainDest, this.terrainWaterDest, new Stamp(this, _output, _transData));
              Stamp stamp = new Stamp(this, _output, _transData, true, (Color) new Color32((byte) 0, (byte) 0, (byte) this.WaterHeight, (byte) 0), _isWater: true);
              this.waterLayer.Stamps.Add(stamp);
              StampManager.DrawWaterStamp(stamp, this.waterDest, this.WorldSize);
            }
          }
        }
      }
      _rand.Free();
      Log.Out("generateBaseStamps terrainBorderThread in {0}", (object) (float) ((double) microStopwatch1.ElapsedMilliseconds * (1.0 / 1000.0)));
    }));
    terrainBorderTask.Start();
    System.Threading.Tasks.Task radTask = new System.Threading.Tasks.Task((System.Action) ([PublicizedFrom(EAccessModifier.Internal)] () =>
    {
      MicroStopwatch microStopwatch = new MicroStopwatch(true);
      Color color = new Color(1f, 0.0f, 0.0f, 0.0f);
      int num = this.WorldSize - 1;
      for (int index = 0; index < this.WorldSize; ++index)
      {
        this.radDest[index] = (Color32) color;
        this.radDest[index + num * this.WorldSize] = (Color32) color;
        this.radDest[index * this.WorldSize] = (Color32) color;
        this.radDest[index * this.WorldSize + num] = (Color32) color;
      }
      Log.Out("generateBaseStamps radThread in {0}", (object) (float) ((double) microStopwatch.ElapsedMilliseconds * (1.0 / 1000.0)));
    }));
    radTask.Start();
    System.Threading.Tasks.Task[] biomeTasks = new System.Threading.Tasks.Task[1]
    {
      new System.Threading.Tasks.Task((System.Action) ([PublicizedFrom(EAccessModifier.Internal)] () =>
      {
        MicroStopwatch microStopwatch = new MicroStopwatch(true);
        Rand _rand = new Rand(this.Seed + 3);
        Color32 biomeColor = this.biomeColors[BiomeType.forest];
        for (int index = 0; index < this.biomeDest.Length; ++index)
          this.biomeDest[index] = biomeColor;
        RawStamp stamp = this.StampManager.GetStamp("filler_biome", _rand);
        if (stamp != null)
        {
          int num1 = this.WorldSize / 256 /*0x0100*/;
          int num2 = 32 /*0x20*/;
          int num3 = num2 / 2;
          float num4 = (float) ((double) num2 / (double) stamp.width * 1.5);
          for (int index4 = 0; index4 < num1; ++index4)
          {
            int num5 = index4 * 256 /*0x0100*/ / 8;
            for (int index5 = 0; index5 < num1; ++index5)
            {
              int num6 = index5 * 256 /*0x0100*/ / 8;
              BiomeType key = this.biomeMap.data[index5, index4];
              if (key != BiomeType.none)
              {
                float _scale = num4 + _rand.Range(0.0f, 0.2f);
                float _angle = (float) (_rand.Range(0, 4) * 90 + _rand.Range(-20, 20));
                StampManager.DrawBiomeStamp(this.biomeDest, stamp.alphaPixels, num6 + num3, num5 + num3, this.BiomeSize, this.BiomeSize, stamp.width, stamp.height, _scale, this.biomeColors[key], _angle: _angle);
              }
            }
          }
        }
        _rand.Free();
        Log.Out("generateBaseStamps biomeThreads in {0}", (object) (float) ((double) microStopwatch.ElapsedMilliseconds * (1.0 / 1000.0)));
      }))
    };
    foreach (System.Threading.Tasks.Task task in biomeTasks)
      task.Start();
    bool isAnyAlive = true;
    while (isAnyAlive || !terrainBorderTask.IsCompleted || !radTask.IsCompleted)
    {
      isAnyAlive = false;
      foreach (System.Threading.Tasks.Task task in biomeTasks)
        isAnyAlive |= !task.IsCompleted;
      if (!terrainBorderTask.IsCompleted & isAnyAlive)
        yield return (object) this.SetMessage(Localization.Get("xuiRwgCreatingTerrainAndBiomeStamps"));
      else if (!terrainBorderTask.IsCompleted && !isAnyAlive)
        yield return (object) this.SetMessage(Localization.Get("xuiRwgCreatingTerrainStamps"));
      else
        yield return (object) this.SetMessage(Localization.Get("xuiRwgCreatingBiomeStamps"));
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public IEnumerator GenerateTerrainFromTiles(TerrainType _terrainType, int _tileSize)
  {
    WorldBuilder _worldBuilder1 = this;
    Log.Out("GenerateTerrainFromTiles {0}, start at {1}, r={2:x}", (object) _terrainType, (object) (float) ((double) _worldBuilder1.totalMS.ElapsedMilliseconds * (1.0 / 1000.0)), (object) Rand.Instance.PeekSample());
    int widthInTiles = _worldBuilder1.WorldSize / 256 /*0x0100*/;
    int t = 0;
    string terrainTypeName = _terrainType.ToStringCached<TerrainType>();
    int step = _tileSize / 256 /*0x0100*/;
    for (int tileX = 0; tileX < widthInTiles; tileX += step)
    {
      for (int tileY = 0; tileY < widthInTiles; tileY += step)
      {
        if (_worldBuilder1.IsMessageElapsed())
          yield return (object) _worldBuilder1.SetMessage(string.Format(Localization.Get("xuiRwgGeneratingTerrain"), (object) Mathf.FloorToInt((float) (100.0 * ((double) t / (double) (widthInTiles * widthInTiles))))));
        ++t;
        bool flag1 = true;
        for (int index1 = 0; index1 < step; ++index1)
        {
          for (int index2 = 0; index2 < step; ++index2)
          {
            if (_worldBuilder1.terrainTypeMap.data[tileX + index1, tileY + index2] == _terrainType)
            {
              flag1 = false;
              break;
            }
          }
          if (!flag1)
            break;
        }
        if (!flag1)
        {
          BiomeType biomeType = _worldBuilder1.biomeMap.data[tileX, tileY];
          if (biomeType == BiomeType.none)
            biomeType = BiomeType.forest;
          if (_terrainType == TerrainType.mountains && biomeType == BiomeType.wasteland)
          {
            _worldBuilder1.terrainTypeMap.data[tileX, tileY] = TerrainType.plains;
          }
          else
          {
            int num1 = tileX * 256 /*0x0100*/ + _tileSize / 2;
            int num2 = tileY * 256 /*0x0100*/ + _tileSize / 2;
            string stringCached = biomeType.ToStringCached<BiomeType>();
            string comboTypeName = $"{stringCached}_{terrainTypeName}";
            Vector2 _scaleMinMax;
            int _clusterCount;
            int _clusterRadius;
            float _clusterStrength;
            bool useBiomeMask;
            float biomeCutoff;
            _worldBuilder1.GetTerrainProperties(stringCached, terrainTypeName, comboTypeName, out _scaleMinMax, out _clusterCount, out _clusterRadius, out _clusterStrength, out useBiomeMask, out biomeCutoff);
            _scaleMinMax *= (float) step;
            _clusterRadius *= step;
            int num3 = 0;
            float num4 = _clusterStrength;
            bool flag2 = false;
            for (int index = 0; index < _clusterCount; ++index)
            {
              RawStamp tmp;
              if (_worldBuilder1.StampManager.TryGetStamp(terrainTypeName, comboTypeName, out tmp))
              {
                Vector2 vector2 = Rand.Instance.RandomOnUnitCircle() * (float) num3;
                TranslationData _transData1 = new TranslationData(num1 + Mathf.RoundToInt(vector2.x), num2 + Mathf.RoundToInt(vector2.y), _scaleMinMax.x, _scaleMinMax.y);
                WorldBuilder _worldBuilder2 = _worldBuilder1;
                RawStamp _stamp = tmp;
                TranslationData _transData2 = _transData1;
                string name = tmp.name;
                Color _customColor = new Color();
                string stampName = name;
                _worldBuilder1.terrainLayer.Stamps.Add(new Stamp(_worldBuilder2, _stamp, _transData2, _customColor: _customColor, stampName: stampName)
                {
                  alpha = num4,
                  additive = flag2
                });
                if (tmp.hasWater)
                  _worldBuilder1.waterLayer.Stamps.Add(new Stamp(_worldBuilder1, tmp, _transData1, _isWater: true));
                if (useBiomeMask)
                  _worldBuilder1.biomeLayer.Stamps.Add(new Stamp(_worldBuilder1, tmp, _transData1, true, (Color) _worldBuilder1.biomeColors[biomeType], biomeCutoff));
                num3 = _clusterRadius;
                num4 = _clusterStrength * 0.45f;
                flag2 = true;
              }
            }
          }
        }
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public IEnumerator writeStampsToMaps()
  {
    WorldBuilder worldBuilder = this;
    System.Threading.Tasks.Task terraTask = new System.Threading.Tasks.Task(new System.Action(worldBuilder.\u003CwriteStampsToMaps\u003Eb__118_0));
    System.Threading.Tasks.Task biomeTask = new System.Threading.Tasks.Task(new System.Action(worldBuilder.\u003CwriteStampsToMaps\u003Eb__118_1));
    System.Threading.Tasks.Task radnwatTask = new System.Threading.Tasks.Task(new System.Action(worldBuilder.\u003CwriteStampsToMaps\u003Eb__118_2));
    terraTask.Start();
    biomeTask.Start();
    radnwatTask.Start();
    while (!terraTask.IsCompleted || !biomeTask.IsCompleted || !radnwatTask.IsCompleted)
      yield return (object) worldBuilder.SetMessage(Localization.Get("xuiRwgWritingStampsToMap"));
    Log.Out("writeStampsToMaps end at {0}, r={1:x}", (object) (float) ((double) worldBuilder.totalMS.ElapsedMilliseconds * (1.0 / 1000.0)), (object) Rand.Instance.PeekSample());
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public IEnumerator GenerateTerrainLast()
  {
    WorldBuilder worldBuilder = this;
    MicroStopwatch ms = new MicroStopwatch(true);
    worldBuilder.StampManager.GetStamp("base");
    if (worldBuilder.Lakes > WorldBuilder.GenerationSelections.None)
      worldBuilder.generateTerrainFeature("lake", worldBuilder.Lakes, true);
    if (worldBuilder.Rivers > WorldBuilder.GenerationSelections.None)
      worldBuilder.generateTerrainFeature("river", worldBuilder.Rivers, true);
    if (worldBuilder.Canyons > WorldBuilder.GenerationSelections.None)
      worldBuilder.generateTerrainFeature("canyon", worldBuilder.Canyons);
    if (worldBuilder.Craters > WorldBuilder.GenerationSelections.None)
      worldBuilder.generateTerrainFeature("crater", worldBuilder.Craters);
    MicroStopwatch ms2 = new MicroStopwatch(true);
    System.Threading.Tasks.Task terrainTask = new System.Threading.Tasks.Task(new System.Action(worldBuilder.\u003CGenerateTerrainLast\u003Eb__119_0));
    System.Threading.Tasks.Task waterTask = new System.Threading.Tasks.Task(new System.Action(worldBuilder.\u003CGenerateTerrainLast\u003Eb__119_1));
    terrainTask.Start();
    waterTask.Start();
    while (!terrainTask.IsCompleted || !waterTask.IsCompleted)
    {
      if (!terrainTask.IsCompleted && waterTask.IsCompleted)
        yield return (object) worldBuilder.SetMessage(Localization.Get("xuiRwgWritingTerrainStampsToMap"));
      else if (terrainTask.IsCompleted && !waterTask.IsCompleted)
        yield return (object) worldBuilder.SetMessage(Localization.Get("xuiRwgWritingWaterStampsToMap"));
      else
        yield return (object) worldBuilder.SetMessage(Localization.Get("xuiRwgWritingTerrainAndWaterStampsToMap"));
    }
    ms2.ResetAndRestart();
    System.Threading.Tasks.Task waterMapTask = new System.Threading.Tasks.Task(new System.Action(worldBuilder.\u003CGenerateTerrainLast\u003Eb__119_2));
    waterMapTask.Start();
    while (!waterMapTask.IsCompleted)
      yield return (object) worldBuilder.SetMessage(Localization.Get("xuiRwgCleaningUpWaterMapData"));
    Log.Out("GenerateTerrainLast done in {0}, r={1:x}", (object) (float) ((double) ms.ElapsedMilliseconds * (1.0 / 1000.0)), (object) Rand.Instance.PeekSample());
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public IEnumerator FinalizeWater()
  {
    MicroStopwatch ms = new MicroStopwatch(true);
    MicroStopwatch msReset = new MicroStopwatch(true);
    for (int i = 0; i < this.waterRects.Count; ++i)
    {
      Rect waterRect = this.waterRects[i];
      int num1 = Utils.FastMax(Mathf.FloorToInt(waterRect.min.x), 0);
      waterRect = this.waterRects[i];
      int num2 = Utils.FastMin(Mathf.FloorToInt(waterRect.max.x), this.WorldSize - 1);
      waterRect = this.waterRects[i];
      int num3 = Utils.FastMax(Mathf.FloorToInt(waterRect.min.y), 0);
      waterRect = this.waterRects[i];
      int num4 = Utils.FastMin(Mathf.FloorToInt(waterRect.max.y), this.WorldSize - 1);
      for (int index1 = num3; index1 <= num4; ++index1)
      {
        for (int index2 = num1; index2 <= num2; ++index2)
        {
          int index3 = index2 + index1 * this.WorldSize;
          if ((double) this.HeightMap[index3] - 0.5 > (double) this.WaterMap[index3])
          {
            this.waterDest[index3] = 0.0f;
            this.WaterMap[index3] = (byte) 0;
          }
          else
          {
            this.WaterMap[index3] = (byte) this.WaterHeight;
            this.waterDest[index3] = (float) this.WaterHeight / (float) byte.MaxValue;
          }
        }
      }
      if (msReset.ElapsedMilliseconds > 500L)
      {
        yield return (object) null;
        msReset.ResetAndRestart();
      }
    }
    Log.Out("FinalizeWater in {0}", (object) (float) ((double) ms.ElapsedMilliseconds * (1.0 / 1000.0)));
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void serializeWater(Stream stream)
  {
    MicroStopwatch microStopwatch = new MicroStopwatch(true);
    Color32[] color32Array = new Color32[this.WorldSize * this.WorldSize];
    for (int index1 = 0; index1 < this.WorldSize; ++index1)
    {
      for (int index2 = 0; index2 < this.WorldSize; ++index2)
        color32Array[index1 * this.WorldSize + index2] = new Color32((byte) 0, (byte) 0, (byte) ((double) this.waterDest[index1 * this.WorldSize + index2] * (double) byte.MaxValue), (byte) 0);
    }
    Log.Out($"Create water in {(ValueType) (float) ((double) microStopwatch.ElapsedMilliseconds * (1.0 / 1000.0))}");
    stream.Write(ReadOnlySpan<byte>.op_Implicit(ImageConversion.EncodeArrayToPNG((Array) color32Array, GraphicsFormat.R8G8B8A8_UNorm, (uint) this.WorldSize, (uint) this.WorldSize, (uint) (this.WorldSize * 4))));
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void generateTerrainFeature(
    string featureName,
    WorldBuilder.GenerationSelections selection,
    bool isWaterFeature = false)
  {
    MicroStopwatch microStopwatch = new MicroStopwatch(true);
    Vector2 vector2 = new Vector2(0.5f, 1.5f);
    Vector2i zero1 = Vector2i.zero;
    Vector2i zero2 = Vector2i.zero;
    Vector2i zero3 = Vector2i.zero;
    Vector2i zero4 = Vector2i.zero;
    Vector2 zero5 = Vector2.zero;
    Vector2 zero6 = Vector2.zero;
    GameRandom gameRandom = GameRandomManager.Instance.CreateGameRandom(this.Seed + featureName.GetHashCode() + 1);
    GameRandom rnd2 = GameRandomManager.Instance.CreateGameRandom(this.Seed + featureName.GetHashCode() + 2);
    string _input;
    if ((_input = this.thisWorldProperties.GetString(featureName + "s.scale")) != string.Empty)
      vector2 = StringParsers.ParseVector2(_input);
    int count = this.GetCount(featureName + "s", selection);
    for (int index1 = 0; index1 < count; ++index1)
    {
      RawStamp _output;
      if (!this.StampManager.TryGetStamp(featureName, out _output))
      {
        if (featureName.Contains("river"))
          Log.Out("Could not find stamp {0}", (object) featureName);
      }
      else
      {
        float _scale = gameRandom.RandomRange(vector2.x, vector2.y) * 1.4f;
        int num1 = gameRandom.RandomRange(0, 360);
        int num2 = (int) ((double) _output.width * (double) _scale);
        int num3 = (int) ((double) _output.height * (double) _scale);
        int x1 = -(num2 / 2);
        int y1 = -(num3 / 2);
        Vector2i rotatedPoint1 = this.getRotatedPoint(x1, y1, x1 + num2 / 2, y1 + num3 / 2, num1);
        Vector2i rotatedPoint2 = this.getRotatedPoint(x1 + num2, y1, x1 + num2 / 2, y1 + num3 / 2, num1);
        Vector2i rotatedPoint3 = this.getRotatedPoint(x1, y1 + num3, x1 + num2 / 2, y1 + num3 / 2, num1);
        Vector2i rotatedPoint4 = this.getRotatedPoint(x1 + num2, y1 + num3, x1 + num2 / 2, y1 + num3 / 2, num1);
        zero5.x = (float) Mathf.Min(Mathf.Min(rotatedPoint1.x, rotatedPoint2.x), Mathf.Min(rotatedPoint3.x, rotatedPoint4.x));
        zero5.y = (float) Mathf.Min(Mathf.Min(rotatedPoint1.y, rotatedPoint2.y), Mathf.Min(rotatedPoint3.y, rotatedPoint4.y));
        zero6.x = (float) Mathf.Max(Mathf.Max(rotatedPoint1.x, rotatedPoint2.x), Mathf.Max(rotatedPoint3.x, rotatedPoint4.x));
        zero6.y = (float) Mathf.Max(Mathf.Max(rotatedPoint1.y, rotatedPoint2.y), Mathf.Max(rotatedPoint3.y, rotatedPoint4.y));
        Rect rect = new Rect(zero5, zero6 - zero5);
        using (List<StreetTile>.Enumerator enumerator = ((IEnumerable) this.StreetTileMap).Cast<StreetTile>().Where<StreetTile>((Func<StreetTile, bool>) ([PublicizedFrom(EAccessModifier.Internal)] (st) => (st.Township == null || st.District == null || st.District.name == "wilderness") && st.TerrainType != TerrainType.mountains && !st.HasFeature && st.GetNeighborCount() > 3)).OrderBy<StreetTile, int>((Func<StreetTile, int>) ([PublicizedFrom(EAccessModifier.Internal)] (st) => rnd2.RandomInt)).ToList<StreetTile>().GetEnumerator())
        {
label_44:
          while (enumerator.MoveNext())
          {
            StreetTile current = enumerator.Current;
            if (current.GridPosition.x != 0 && current.GridPosition.y != 0)
            {
              for (int x2 = current.WorldPositionCenter.x - (int) rect.width / 2; (double) x2 < (double) current.WorldPositionCenter.x + (double) rect.width / 2.0; x2 += 150)
              {
                for (int y2 = current.WorldPositionCenter.y - (int) rect.height / 2; (double) y2 < (double) current.WorldPositionCenter.y + (double) rect.height / 2.0; y2 += 150)
                {
                  StreetTile streetTileWorld = this.GetStreetTileWorld(x2, y2);
                  if (streetTileWorld == null || streetTileWorld.Township != null || streetTileWorld.District != null || streetTileWorld.Used || streetTileWorld.HasFeature)
                    goto label_44;
                }
              }
              for (int x3 = current.WorldPositionCenter.x - (int) rect.width / 2; (double) x3 < (double) current.WorldPositionCenter.x + (double) rect.width / 2.0; x3 += 150)
              {
                for (int y3 = current.WorldPositionCenter.y - (int) rect.height / 2; (double) y3 < (double) current.WorldPositionCenter.y + (double) rect.height / 2.0; y3 += 150)
                  this.GetStreetTileWorld(x3, y3).HasFeature = true;
              }
              TranslationData _transData = new TranslationData(current.WorldPositionCenter.x, current.WorldPositionCenter.y, _scale, num1);
              Stamp stamp = new Stamp(this, _output, _transData);
              if (isWaterFeature)
              {
                bool flag = false;
                for (int index2 = 0; index2 < this.terrainLayer.Stamps.Count; ++index2)
                {
                  if (stamp.Name.Contains("mountain") && stamp.Area.Overlaps(this.terrainLayer.Stamps[index2].Area))
                  {
                    flag = true;
                    break;
                  }
                }
                if (flag)
                {
                  --index1;
                }
                else
                {
                  this.lowerLayer.Stamps.Add(stamp);
                  this.waterLayer.Stamps.Add(new Stamp(this, _output, _transData, true, (Color) new Color32((byte) 0, (byte) 0, (byte) this.WaterHeight, (byte) 0), _isWater: true));
                  break;
                }
              }
              else
              {
                this.lowerLayer.Stamps.Add(stamp);
                bool flag = true;
                for (int index3 = 0; index3 < this.waterLayer.Stamps.Count; ++index3)
                {
                  if (stamp.Area.Overlaps(this.waterLayer.Stamps[index3].Area))
                  {
                    flag = false;
                    break;
                  }
                }
                if (flag)
                {
                  for (int index4 = 0; index4 < this.waterRects.Count; ++index4)
                  {
                    if (stamp.Area.Overlaps(this.waterRects[index4]))
                    {
                      flag = false;
                      break;
                    }
                  }
                }
                if (!flag)
                {
                  this.waterLayer.Stamps.Add(new Stamp(this, _output, _transData, true, (Color) new Color32((byte) 0, (byte) 0, (byte) this.WaterHeight, (byte) 0), 0.05f, true));
                  break;
                }
                break;
              }
            }
          }
        }
      }
    }
    GameRandomManager.Instance.FreeGameRandom(rnd2);
    GameRandomManager.Instance.FreeGameRandom(gameRandom);
    Log.Out("generateTerrainFeature {0} in {1}", (object) featureName, (object) (float) ((double) microStopwatch.ElapsedMilliseconds * (1.0 / 1000.0)));
  }

  public bool CreatePlayerSpawn(Vector2i worldPos, bool _isFallback = false)
  {
    Vector3 _position = new Vector3((float) worldPos.x, this.GetHeight(worldPos), (float) worldPos.y);
    if (!_isFallback)
    {
      for (int index = 0; index < this.playerSpawns.Count; ++index)
      {
        if (this.playerSpawns[index].IsTooClose(_position))
          return false;
      }
      StreetTile streetTileWorld = this.GetStreetTileWorld(worldPos);
      if (streetTileWorld != null && streetTileWorld.HasPrefabs)
      {
        if (this.ForestBiomeWeight > 0 && streetTileWorld.BiomeType != BiomeType.forest)
          return false;
        foreach (PrefabDataInstance streetTilePrefabData in streetTileWorld.StreetTilePrefabDatas)
        {
          if (streetTilePrefabData.prefab.DifficultyTier >= 2)
            return false;
        }
      }
      List<Vector2i> vector2iList = this.ForestBiomeWeight > 0 ? this.TraderForestCenterPositions : this.TraderCenterPositions;
      bool flag = false;
      for (int index = 0; index < vector2iList.Count; ++index)
      {
        if ((double) Vector2i.DistanceSqr(vector2iList[index], worldPos) < 810000.0)
        {
          flag = true;
          break;
        }
      }
      if (!flag)
        return false;
    }
    this.playerSpawns.Add(new WorldBuilder.PlayerSpawn(_position, (float) Rand.Instance.Range(0, 360)));
    return true;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void CalcTownshipsHeightMask()
  {
    int worldSize1 = this.WorldSize;
    int worldSize2 = this.WorldSize;
    this.poiHeightMask = new byte[worldSize1 * worldSize2];
    if (this.Townships == null)
      return;
    for (int index1 = 0; index1 < this.Townships.Count; ++index1)
    {
      foreach (StreetTile streetTile in this.Townships[index1].Streets.Values)
      {
        int num1 = 0;
        int num2 = streetTile.WorldPosition.x + streetTile.WorldPosition.y * worldSize1;
        for (int index2 = -num1; index2 < 150 + num1; ++index2)
        {
          for (int index3 = -num1; index3 < 150 + num1; ++index3)
            this.poiHeightMask[index3 + num2] = (byte) 1;
          num2 += worldSize1;
        }
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void CalcWindernessPOIsHeightMask(Color32[] roadMask)
  {
    int worldSize = this.WorldSize;
    for (int index1 = 0; index1 < this.StreetTileMapSize; ++index1)
    {
      for (int index2 = 0; index2 < this.StreetTileMapSize; ++index2)
      {
        StreetTile streetTile = this.StreetTileMap[index2, index1];
        if (streetTile.NeedsWildernessSmoothing)
        {
          int num = streetTile.WildernessPOIPos.x + streetTile.WildernessPOIPos.y * worldSize;
          for (int index3 = 0; index3 < streetTile.WildernessPOISize.y; ++index3)
          {
            for (int index4 = 0; index4 < streetTile.WildernessPOISize.x; ++index4)
              this.poiHeightMask[index4 + num] = (byte) 1;
            num += worldSize;
          }
        }
      }
    }
  }

  public IEnumerator SmoothRoadTerrain(
    Color32[] roadMask,
    float[] HeightMap,
    int WorldSize,
    List<Township> _townships = null)
  {
    MicroStopwatch ms = new MicroStopwatch(true);
    yield return (object) null;
    int width = WorldSize;
    int height = WorldSize;
    int len = width * height;
    ushort[] mask = new ushort[len];
    for (int index1 = 0; index1 < height; ++index1)
    {
      for (int index2 = 0; index2 < width; ++index2)
      {
        int index3 = index2 + index1 * width;
        if (this.poiHeightMask[index3] > (byte) 0)
        {
          mask[index3] = (ushort) 1000;
        }
        else
        {
          int r = (int) roadMask[index3].r;
          if (r + (int) roadMask[index3].g > 0)
          {
            HeightMap[index3] += 0.0008f;
            mask[index3] = (ushort) 200;
            int num1 = 80 /*0x50*/;
            int num2 = 3;
            int num3 = 30;
            if (r > 0)
            {
              mask[index3] = (ushort) byte.MaxValue;
              num1 = 60;
              num2 = 6;
              num3 = 8;
            }
            for (int index4 = 1; index4 <= num2; ++index4)
            {
              for (int index5 = 0; index5 < 8; ++index5)
              {
                int num4 = index2 + this.directions8way[index5].x * index4;
                if ((long) (uint) num4 < (long) width)
                {
                  int num5 = index1 + this.directions8way[index5].y * index4;
                  if ((long) (uint) num5 < (long) height)
                  {
                    int index6 = num4 + num5 * width;
                    if (num1 > (int) mask[index6])
                      mask[index6] = (ushort) num1;
                  }
                }
              }
              num1 -= num3;
            }
          }
        }
      }
    }
    yield return (object) null;
    int messageCnt = 0;
    int clampX = width - 1;
    int clampY = height - 1;
    float[] heights = new float[len];
    int highwayPasses = 6;
    while (highwayPasses-- > 0)
    {
      Array.Copy((Array) HeightMap, (Array) heights, len);
      for (int index7 = 1; index7 < clampY; ++index7)
      {
        int num6 = index7 * width;
        for (int index8 = 1; index8 < clampX; ++index8)
        {
          int index9 = index8 + num6;
          if (roadMask[index9].r != (byte) 0 && mask[index9] < (ushort) 1000)
          {
            double num7;
            float num8 = (float) (num7 = (double) heights[index9]);
            int index10 = index9 - width - 1;
            float num9 = (float) num7;
            if (mask[index10] >= (ushort) byte.MaxValue)
              num9 = heights[index10];
            float num10 = num8 + num9 * 0.25f;
            float num11 = (float) num7;
            int index11;
            if (mask[index11 = index10 + 1] >= (ushort) byte.MaxValue)
              num11 = heights[index11];
            float num12 = num10 + num11 * 0.5f;
            float num13 = (float) num7;
            int index12;
            if (mask[index12 = index11 + 1] >= (ushort) byte.MaxValue)
              num13 = heights[index12];
            float num14 = num12 + num13 * 0.25f;
            int index13 = index9 - 1;
            float num15 = (float) num7;
            if (mask[index13] >= (ushort) byte.MaxValue)
              num15 = heights[index13];
            float num16 = num14 + num15 * 0.5f;
            int index14 = index13 + 2;
            float num17 = (float) num7;
            if (mask[index14] >= (ushort) byte.MaxValue)
              num17 = heights[index14];
            float num18 = num16 + num17 * 0.5f;
            int index15 = index9 + width - 1;
            float num19 = (float) num7;
            if (mask[index15] >= (ushort) byte.MaxValue)
              num19 = heights[index15];
            float num20 = num18 + num19 * 0.25f;
            float num21 = (float) num7;
            int index16;
            if (mask[index16 = index15 + 1] >= (ushort) byte.MaxValue)
              num21 = heights[index16];
            float num22 = num20 + num21 * 0.5f;
            float num23 = (float) num7;
            int index17;
            if (mask[index17 = index16 + 1] >= (ushort) byte.MaxValue)
              num23 = heights[index17];
            float num24 = num22 + num23 * 0.25f;
            HeightMap[index9] = num24 / 4f;
          }
        }
      }
      if (this.IsMessageElapsed())
        yield return (object) this.SetMessage(string.Format(Localization.Get("xuiRwgSmoothRoadTerrainCount"), (object) ++messageCnt));
    }
    messageCnt = 100;
    int roadAndAdjacentPasses = 30;
    while (roadAndAdjacentPasses-- > 0)
    {
      Array.Copy((Array) HeightMap, (Array) heights, len);
      for (int index18 = 1; index18 < clampY; ++index18)
      {
        int num25 = index18 * width;
        for (int index19 = 1; index19 < clampX; ++index19)
        {
          int index20 = index19 + num25;
          int num26 = (int) mask[index20];
          if (num26 != 0 && num26 <= 200)
          {
            int num27 = 0;
            float num28 = 0.0f;
            int index21 = index20 - width - 1;
            int num29 = (int) mask[index21] / 2;
            float num30 = num28 + heights[index21] * (float) num29;
            int num31 = num27 + num29;
            int index22;
            int num32 = (int) mask[index22 = index21 + 1];
            float num33 = num30 + heights[index22] * (float) num32;
            int num34 = num31 + num32;
            int index23;
            int num35 = (int) mask[index23 = index22 + 1] / 2;
            float num36 = num33 + heights[index23] * (float) num35;
            int num37 = num34 + num35;
            int index24 = index20 - 1;
            int num38 = (int) mask[index24];
            float num39 = num36 + heights[index24] * (float) num38;
            int num40 = num37 + num38;
            int index25 = index20 + 1;
            int num41 = (int) mask[index25];
            float num42 = num39 + heights[index25] * (float) num41;
            int num43 = num40 + num41;
            int index26 = index20 + width - 1;
            int num44 = (int) mask[index26] / 2;
            float num45 = num42 + heights[index26] * (float) num44;
            int num46 = num43 + num44;
            int index27;
            int num47 = (int) mask[index27 = index26 + 1];
            float num48 = num45 + heights[index27] * (float) num47;
            int num49 = num46 + num47;
            int index28;
            int num50 = (int) mask[index28 = index27 + 1] / 2;
            float num51 = num48 + heights[index28] * (float) num50;
            int num52 = num49 + num50;
            if (num52 > 0)
            {
              if (num26 < 200)
              {
                float num53 = (float) num26 * 0.005f;
                HeightMap[index20] = (float) ((double) HeightMap[index20] * (1.0 - (double) num53) + (double) num51 / (double) num52 * (double) num53);
              }
              else
                HeightMap[index20] = num51 / (float) num52;
            }
          }
        }
      }
      if (this.IsMessageElapsed())
        yield return (object) this.SetMessage(string.Format(Localization.Get("xuiRwgSmoothRoadTerrainCount"), (object) ++messageCnt));
    }
    Log.Out("Smooth Road Terrain in {0}", (object) (float) ((double) ms.ElapsedMilliseconds * (1.0 / 1000.0)));
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public IEnumerator SmoothWildernessTerrain()
  {
    yield return (object) null;
    MicroStopwatch microStopwatch = new MicroStopwatch(true);
    foreach (StreetTile streetTile in this.getWildernessTilesToSmooth())
      streetTile.SmoothWildernessTerrain();
    Log.Out($"Smooth Wilderness Terrain in {(ValueType) (float) ((double) microStopwatch.ElapsedMilliseconds * (1.0 / 1000.0))}, r={Rand.Instance.PeekSample():x}");
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void GenerateTerrainTiles()
  {
    this.terrainTypeMap = new DataMap<TerrainType>(this.WorldSize / 256 /*0x0100*/, TerrainType.none);
    Rand instance = Rand.Instance;
    List<TileGroup> tileGroupList = new List<TileGroup>();
    for (int index = 0; index < 5; ++index)
      tileGroupList.Add(new TileGroup()
      {
        Biome = (BiomeType) index
      });
    for (int _x = 0; _x < this.biomeMap.data.GetLength(0); ++_x)
    {
      for (int _y = 0; _y < this.biomeMap.data.GetLength(1); ++_y)
      {
        Vector2i vector2i = new Vector2i(_x, _y);
        BiomeType index = this.biomeMap.data[_x, _y];
        tileGroupList[(int) index].Positions.Add(vector2i);
      }
    }
    float num1 = (float) (this.Plains + this.Hills + this.Mountains);
    if ((double) num1 == 0.0)
    {
      this.Plains = 1;
      num1 = 1f;
    }
    foreach (TileGroup tileGroup in tileGroupList)
    {
      int num2 = Mathf.FloorToInt((float) this.Plains / num1 * (float) tileGroup.Positions.Count);
      int num3 = Mathf.FloorToInt((float) this.Hills / num1 * (float) tileGroup.Positions.Count);
      int num4 = Mathf.FloorToInt((float) this.Mountains / num1 * (float) tileGroup.Positions.Count);
      while (tileGroup.Positions.Count > num2 + num3 + num4)
      {
        int num5 = instance.Range(3);
        if (num5 == 0)
        {
          if (this.Plains > 0)
            ++num2;
          else
            ++num5;
        }
        if (num5 == 1)
        {
          if (this.Hills > 0)
            ++num3;
          else
            ++num5;
        }
        if (num5 == 2 && this.Mountains > 0)
          ++num4;
      }
      int index1 = instance.Range(tileGroup.Positions.Count);
      while (tileGroup.Positions.Count > 0)
      {
        Vector2i position = tileGroup.Positions[index1];
        tileGroup.Positions.RemoveAt(index1);
        index1 = instance.Range(tileGroup.Positions.Count);
        int index2 = position.x / 1;
        int index3 = position.y / 1;
        if (this.terrainTypeMap.data[index2, index3] == TerrainType.none)
        {
          if (num3 > 0)
          {
            if (num3 >= 2)
            {
              int index4 = index2 & -2;
              int index5 = index3 & -2;
              this.terrainTypeMap.data[index4, index5] = TerrainType.hills;
              this.terrainTypeMap.data[index4 + 1, index5] = TerrainType.hills;
              this.terrainTypeMap.data[index4, index5 + 1] = TerrainType.hills;
              this.terrainTypeMap.data[index4 + 1, index5 + 1] = TerrainType.hills;
            }
            num3 -= 4;
          }
          else if (num4 > 0)
          {
            --num4;
            this.terrainTypeMap.data[index2, index3] = TerrainType.mountains;
            if (num4 > 0 && (double) instance.Float() < 0.800000011920929)
            {
              int index6 = instance.Range(4);
              for (int index7 = 0; index7 < 4; ++index7)
              {
                index6 = index6 + 1 & 3;
                Vector2i vector2i;
                vector2i.x = position.x + this.directions4way[index6].x;
                vector2i.y = position.y + this.directions4way[index6].y;
                int num6 = tileGroup.Positions.IndexOf(vector2i);
                if (num6 >= 0 && this.terrainTypeMap.data[vector2i.x / 1, vector2i.y / 1] == TerrainType.none)
                {
                  index1 = num6;
                  break;
                }
              }
            }
          }
          else
            this.terrainTypeMap.data[index2, index3] = TerrainType.plains;
        }
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public List<WorldBuilder.BiomeTypeData> CalcBiomeTileBiomeData(int totalTiles)
  {
    float num1 = (float) (this.ForestBiomeWeight + this.BurntForestBiomeWeight + this.DesertBiomeWeight + this.SnowBiomeWeight + this.WastelandBiomeWeight);
    List<WorldBuilder.BiomeTypeData> list = new List<WorldBuilder.BiomeTypeData>()
    {
      new WorldBuilder.BiomeTypeData(BiomeType.forest, (float) this.ForestBiomeWeight / num1, totalTiles),
      new WorldBuilder.BiomeTypeData(BiomeType.burntForest, (float) this.BurntForestBiomeWeight / num1, totalTiles),
      new WorldBuilder.BiomeTypeData(BiomeType.desert, (float) this.DesertBiomeWeight / num1, totalTiles),
      new WorldBuilder.BiomeTypeData(BiomeType.snow, (float) this.SnowBiomeWeight / num1, totalTiles),
      new WorldBuilder.BiomeTypeData(BiomeType.wasteland, (float) this.WastelandBiomeWeight / num1, totalTiles)
    }.Where<WorldBuilder.BiomeTypeData>((Func<WorldBuilder.BiomeTypeData, bool>) ([PublicizedFrom(EAccessModifier.Internal)] (b) => (double) b.Percent > 0.0)).OrderBy<WorldBuilder.BiomeTypeData, float>((Func<WorldBuilder.BiomeTypeData, float>) ([PublicizedFrom(EAccessModifier.Internal)] (b) => -b.Percent)).ToList<WorldBuilder.BiomeTypeData>();
    int num2 = 0;
    for (int index = 0; index < list.Count; ++index)
      num2 += list[index].TileCount;
    int index1 = 0;
    for (int index2 = num2; index2 < totalTiles; ++index2)
    {
      ++list[index1].TileCount;
      index1 = (index1 + 1) % list.Count;
    }
    return list;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void GenerateBiomeTiles()
  {
    int tileWidth = this.WorldSize / 256 /*0x0100*/;
    double num1 = (double) tileWidth * 0.5;
    int totalTiles = tileWidth * tileWidth;
    this.biomeMap = new DataMap<BiomeType>(tileWidth, BiomeType.none);
    List<WorldBuilder.BiomeTypeData> biomeTypeDataList = this.CalcBiomeTileBiomeData(totalTiles);
    BiomeType biomeType = BiomeType.none;
    if (this.biomeLayout == WorldBuilder.BiomeLayout.CenterForest)
      biomeType = BiomeType.forest;
    if (this.biomeLayout == WorldBuilder.BiomeLayout.CenterWasteland)
      biomeType = BiomeType.wasteland;
    int num2 = 1;
    float _x1 = (float) num1;
    float _y1 = (float) num1;
    for (int index1 = 0; index1 < num2; ++index1)
    {
      if (this.biomeLayout == WorldBuilder.BiomeLayout.Line)
      {
        float num3 = (float) tileWidth * 0.4f;
        float f = (float) (Rand.Instance.Range(4) * 90) * ((float) Math.PI / 180f);
        Vector2 a;
        a.x = _x1 - Mathf.Cos(f) * num3;
        a.y = _y1 - Mathf.Sin(f) * num3;
        Vector2 b;
        b.x = _x1 + Mathf.Cos(f) * num3;
        b.y = _y1 + Mathf.Sin(f) * num3;
        float t = 0.0f;
        float num4 = 1f / (float) (biomeTypeDataList.Count - 1);
        for (int index2 = 0; index2 < biomeTypeDataList.Count; ++index2)
        {
          WorldBuilder.BiomeTypeData biomeTypeData = biomeTypeDataList[index2];
          Vector2 vector2 = Vector2.Lerp(a, b, t);
          biomeTypeData.Center = new Vector2i((int) vector2.x, (int) vector2.y);
          --biomeTypeData.TileCount;
          this.biomeMap.data[biomeTypeData.Center.x, biomeTypeData.Center.y] = biomeTypeData.Type;
          t += num4;
        }
      }
      else
      {
        int num5 = biomeTypeDataList.Count - 1;
        if (this.biomeLayout == WorldBuilder.BiomeLayout.Circle)
          num5 = biomeTypeDataList.Count;
        float num6 = (float) Rand.Instance.Angle();
        float num7 = 360f / (float) num5;
        if ((double) Rand.Instance.Float() < 0.5)
          num7 *= -1f;
        for (int index3 = 0; index3 < biomeTypeDataList.Count; ++index3)
        {
          WorldBuilder.BiomeTypeData biomeTypeData = biomeTypeDataList[index3];
          if (biomeTypeData.Type == biomeType)
          {
            biomeTypeData.Center = new Vector2i((int) _x1, (int) _y1);
          }
          else
          {
            float num8 = (float) tileWidth * 0.4f;
            float _x2 = _x1 + Mathf.Cos(num6 * ((float) Math.PI / 180f)) * num8;
            float _y2 = _y1 + Mathf.Sin(num6 * ((float) Math.PI / 180f)) * num8;
            num6 += num7;
            biomeTypeData.Center = new Vector2i((int) _x2, (int) _y2);
          }
          --biomeTypeData.TileCount;
          this.biomeMap.data[biomeTypeData.Center.x, biomeTypeData.Center.y] = biomeTypeData.Type;
        }
      }
      _x1 += 3f;
      _y1 += 2f;
    }
    int num9 = totalTiles - biomeTypeDataList.Count;
    int num10 = 1 + this.WorldSize / 2048 /*0x0800*/;
    int num11;
    do
    {
      num11 = num9;
      for (int index4 = 0; index4 < biomeTypeDataList.Count; ++index4)
      {
        WorldBuilder.BiomeTypeData _b = biomeTypeDataList[index4];
        if (_b.TileCount > 0)
        {
          int _edge = 0;
          if (_b.Type == biomeType)
            _edge = num10;
          int num12 = 1 + (int) ((double) _b.Percent * 4.0);
          for (int index5 = 0; index5 < num12 && this.FindBiomeEmptyAndSet(_b, _edge); ++index5)
          {
            --_b.TileCount;
            --num9;
            if (_b.TileCount <= 0)
              break;
          }
        }
      }
    }
    while (num9 != num11);
    int num13;
    do
    {
      num13 = num9;
      for (int index = 0; index < biomeTypeDataList.Count; ++index)
      {
        WorldBuilder.BiomeTypeData _b = biomeTypeDataList[index];
        if (_b.Type != BiomeType.wasteland && this.FindBiomeEmptyAndSet(_b, 0))
          --num9;
      }
    }
    while (num9 != num13);
    for (int index6 = 0; index6 < this.biomeMap.data.GetLength(0); ++index6)
    {
      for (int index7 = 0; index7 < this.biomeMap.data.GetLength(1); ++index7)
      {
        if (this.biomeMap.data[index6, index7] == BiomeType.none)
          this.biomeMap.data[index6, index7] = BiomeType.wasteland;
      }
    }
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public bool FindBiomeEmptyAndSet(WorldBuilder.BiomeTypeData _b, int _edge)
  {
    int v2 = this.WorldSize / 256 /*0x0100*/ - 1 - _edge;
    for (int index1 = 1; index1 <= 39; ++index1)
    {
      int num1 = Utils.FastMax(_edge, _b.Center.x - index1);
      int num2 = Utils.FastMin(_b.Center.x + index1, v2);
      int num3 = Utils.FastMax(_edge, _b.Center.y - index1);
      int num4 = Utils.FastMin(_b.Center.y + index1, v2);
      for (int index2 = 0; index2 <= index1; ++index2)
      {
        int _y1 = _b.Center.y - index1;
        if (_y1 >= num3)
        {
          int _x1 = _b.Center.x - index2;
          if (_x1 >= num1 && this.biomeMap.data[_x1, _y1] == BiomeType.none && this.HasBiomeNeighbor(_x1, _y1, _b.Type))
          {
            this.biomeMap.data[_x1, _y1] = _b.Type;
            return true;
          }
          int _x2 = _b.Center.x + index2;
          if (_x2 <= num2 && this.biomeMap.data[_x2, _y1] == BiomeType.none && this.HasBiomeNeighbor(_x2, _y1, _b.Type))
          {
            this.biomeMap.data[_x2, _y1] = _b.Type;
            return true;
          }
        }
        int _y2 = _b.Center.y + index1;
        if (_y2 <= num4)
        {
          int _x3 = _b.Center.x - index2;
          if (_x3 >= num1 && this.biomeMap.data[_x3, _y2] == BiomeType.none && this.HasBiomeNeighbor(_x3, _y2, _b.Type))
          {
            this.biomeMap.data[_x3, _y2] = _b.Type;
            return true;
          }
          int _x4 = _b.Center.x + index2;
          if (_x4 <= num2 && this.biomeMap.data[_x4, _y2] == BiomeType.none && this.HasBiomeNeighbor(_x4, _y2, _b.Type))
          {
            this.biomeMap.data[_x4, _y2] = _b.Type;
            return true;
          }
        }
        int _x5 = _b.Center.x - index1;
        if (_x5 >= num1)
        {
          int _y3 = _b.Center.y - index2;
          if (_y3 >= num3 && this.biomeMap.data[_x5, _y3] == BiomeType.none && this.HasBiomeNeighbor(_x5, _y3, _b.Type))
          {
            this.biomeMap.data[_x5, _y3] = _b.Type;
            return true;
          }
          int _y4 = _b.Center.y + index2;
          if (_y4 <= num4 && this.biomeMap.data[_x5, _y4] == BiomeType.none && this.HasBiomeNeighbor(_x5, _y4, _b.Type))
          {
            this.biomeMap.data[_x5, _y4] = _b.Type;
            return true;
          }
        }
        int _x6 = _b.Center.x + index1;
        if (_x6 <= num2)
        {
          int _y5 = _b.Center.y - index2;
          if (_y5 >= num3 && this.biomeMap.data[_x6, _y5] == BiomeType.none && this.HasBiomeNeighbor(_x6, _y5, _b.Type))
          {
            this.biomeMap.data[_x6, _y5] = _b.Type;
            return true;
          }
          int _y6 = _b.Center.y + index2;
          if (_y6 <= num4 && this.biomeMap.data[_x6, _y6] == BiomeType.none && this.HasBiomeNeighbor(_x6, _y6, _b.Type))
          {
            this.biomeMap.data[_x6, _y6] = _b.Type;
            return true;
          }
        }
      }
    }
    return false;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public bool HasBiomeNeighbor(int _x, int _y, BiomeType _biomeType)
  {
    int num = this.WorldSize / 256 /*0x0100*/;
    int index1 = _x - 1;
    if (index1 >= 0 && this.biomeMap.data[index1, _y] == _biomeType)
      return true;
    int index2 = _x + 1;
    if (index2 < num && this.biomeMap.data[index2, _y] == _biomeType)
      return true;
    int index3 = _y - 1;
    if (index3 >= 0 && this.biomeMap.data[_x, index3] == _biomeType)
      return true;
    int index4 = _y + 1;
    return index4 < num && this.biomeMap.data[_x, index4] == _biomeType;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public BiomeType GetBiomeFromNeighbors(int _x, int _y)
  {
    int num = this.WorldSize / 256 /*0x0100*/;
    int index1 = _x - 1;
    if (index1 >= 0)
    {
      BiomeType biomeFromNeighbors = this.biomeMap.data[index1, _y];
      switch (biomeFromNeighbors)
      {
        case BiomeType.wasteland:
        case BiomeType.none:
          break;
        default:
          return biomeFromNeighbors;
      }
    }
    int index2 = _x + 1;
    if (index2 < num)
    {
      BiomeType biomeFromNeighbors = this.biomeMap.data[index2, _y];
      switch (biomeFromNeighbors)
      {
        case BiomeType.wasteland:
        case BiomeType.none:
          break;
        default:
          return biomeFromNeighbors;
      }
    }
    int index3 = _y - 1;
    if (index3 >= 0)
    {
      BiomeType biomeFromNeighbors = this.biomeMap.data[_x, index3];
      switch (biomeFromNeighbors)
      {
        case BiomeType.wasteland:
        case BiomeType.none:
          break;
        default:
          return biomeFromNeighbors;
      }
    }
    int index4 = _y + 1;
    if (index4 < num)
    {
      BiomeType biomeFromNeighbors = this.biomeMap.data[_x, index4];
      switch (biomeFromNeighbors)
      {
        case BiomeType.wasteland:
        case BiomeType.none:
          break;
        default:
          return biomeFromNeighbors;
      }
    }
    return BiomeType.none;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void serializeRWGTTW(Stream stream)
  {
    World _world = new World();
    WorldState worldState = new WorldState();
    worldState.SetFrom(_world, EnumChunkProviderId.ChunkDataDriven);
    worldState.ResetDynamicData();
    worldState.Save(stream);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void serializeDynamicProperties(Stream stream)
  {
    new DynamicProperties()
    {
      Values = {
        ["Scale"] = "1",
        ["HeightMapSize"] = string.Format("{0},{0}", (object) this.WorldSize),
        ["Modes"] = "Survival,SurvivalSP,SurvivalMP,Creative",
        ["FixedWaterLevel"] = "false",
        ["RandomGeneratedWorld"] = "true",
        ["GameVersion"] = Constants.cVersionInformation.SerializableString,
        ["Generation.Seed"] = this.WorldSeedName,
        ["Seed"] = this.Seed.ToString(),
        ["Generation.Towns"] = this.Towns.ToString(),
        ["Generation.Wilderness"] = this.Wilderness.ToString(),
        ["Generation.Lakes"] = this.Lakes.ToString(),
        ["Generation.Rivers"] = this.Rivers.ToString(),
        ["Generation.Cracks"] = this.Canyons.ToString(),
        ["Generation.Craters"] = this.Craters.ToString(),
        ["Generation.Plains"] = this.Plains.ToString(),
        ["Generation.Hills"] = this.Hills.ToString(),
        ["Generation.Mountains"] = this.Mountains.ToString(),
        ["Generation.Forest"] = this.ForestBiomeWeight.ToString(),
        ["Generation.BurntForest"] = this.BurntForestBiomeWeight.ToString(),
        ["Generation.Desert"] = this.DesertBiomeWeight.ToString(),
        ["Generation.Snow"] = this.SnowBiomeWeight.ToString(),
        ["Generation.Wasteland"] = this.WastelandBiomeWeight.ToString()
      }
    }.Save("MapInfo", stream);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public IEnumerator CreatePreviewTexture(Color32[] roadMask)
  {
    yield return (object) this.SetMessage(Localization.Get("xuiRwgCreatingPreview"), true);
    MicroStopwatch msReset = new MicroStopwatch(true);
    Color32[] dest = new Color32[roadMask.Length];
    Color32 color = new Color32((byte) 0, (byte) 0, (byte) 0, byte.MaxValue);
    int destOffsetY = 0;
    int biomeSteps = this.WorldSize / this.BiomeSize;
    int y;
    for (y = 0; y < this.BiomeSize; ++y)
    {
      int num1 = destOffsetY;
      for (int index1 = 0; index1 < this.BiomeSize; ++index1)
      {
        Color32 color32 = this.biomeDest[index1 + y * this.BiomeSize];
        color.r = (byte) ((uint) color32.r / 2U);
        color.g = (byte) ((uint) color32.g / 2U);
        color.b = (byte) ((uint) color32.b / 2U);
        for (int index2 = 0; index2 < biomeSteps; ++index2)
        {
          int num2 = num1 + index2 * this.WorldSize;
          for (int index3 = 0; index3 < biomeSteps; ++index3)
            dest[num2 + index3] = color;
        }
        num1 += biomeSteps;
      }
      destOffsetY += biomeSteps * this.WorldSize;
      if (msReset.ElapsedMilliseconds > 500L)
      {
        yield return (object) null;
        msReset.ResetAndRestart();
      }
    }
    yield return (object) null;
    msReset.ResetAndRestart();
    if (this.Townships != null)
    {
      StampGroup roadLayer = new StampGroup("Road Layer");
      foreach (Township township in this.Townships)
      {
        if (township.Streets.Count > 0)
        {
          foreach (Vector2i key in township.Streets.Keys)
          {
            if (township.Streets[key].Township != null)
              roadLayer.Stamps.AddRange((IEnumerable<Stamp>) township.Streets[key].GetStamps());
          }
        }
        if (msReset.ElapsedMilliseconds > 500L)
        {
          yield return (object) null;
          msReset.ResetAndRestart();
        }
      }
      this.StampManager.DrawStampGroup(roadLayer, dest, this.WorldSize);
      roadLayer = (StampGroup) null;
    }
    yield return (object) null;
    msReset.ResetAndRestart();
    Color32 waterColor = new Color32((byte) 0, (byte) 0, byte.MaxValue, byte.MaxValue);
    Color32 radColor = new Color32(byte.MaxValue, (byte) 0, (byte) 0, byte.MaxValue);
    for (y = 0; y < roadMask.Length; ++y)
    {
      int num3 = y % this.WorldSize;
      int num4 = y / this.WorldSize;
      if (roadMask[y].a > (byte) 0)
        dest[y] = roadMask[y];
      if (this.GetWater(y) > (byte) 0)
        dest[y] = waterColor;
      if (this.GetRad(y) > (byte) 0)
        dest[y] = radColor;
      if (y % 50000 == 0 && msReset.ElapsedMilliseconds > 500L)
      {
        yield return (object) null;
        msReset.ResetAndRestart();
      }
    }
    Color32 color32_1 = new Color32((byte) 200, (byte) 200, byte.MaxValue, byte.MaxValue);
    Color32 color32_2 = new Color32((byte) 0, (byte) 0, (byte) 50, byte.MaxValue);
    for (int index4 = 0; index4 < this.playerSpawns.Count; ++index4)
    {
      WorldBuilder.PlayerSpawn playerSpawn = this.playerSpawns[index4];
      int index5 = (int) playerSpawn.Position.x + (int) playerSpawn.Position.z * this.WorldSize;
      dest[index5 - this.WorldSize - 1] = color32_2;
      dest[index5 - this.WorldSize] = color32_1;
      dest[index5 - this.WorldSize + 1] = color32_2;
      dest[index5 - 1] = color32_1;
      dest[index5] = color32_2;
      dest[index5 + 1] = color32_1;
      dest[index5 + this.WorldSize - 1] = color32_2;
      dest[index5 + this.WorldSize] = color32_1;
      dest[index5 + this.WorldSize + 1] = color32_2;
    }
    yield return (object) null;
    if (this.WorldSize >= 0)
    {
      XUiC_WorldGenerationWindowGroup.PreviewQuality previewQualityLevel = XUiC_WorldGenerationWindowGroup.Instance.PreviewQualityLevel;
      if (previewQualityLevel == XUiC_WorldGenerationWindowGroup.PreviewQuality.NoPreview)
      {
        UnityEngine.Object.Destroy((UnityEngine.Object) this.PreviewImage);
        this.PreviewImage = new Texture2D(1, 1);
      }
      else
      {
        UnityEngine.Object.Destroy((UnityEngine.Object) this.PreviewImage);
        this.PreviewImage = new Texture2D(this.WorldSize, this.WorldSize);
        this.PreviewImage.SetPixels32(dest);
        if (previewQualityLevel >= XUiC_WorldGenerationWindowGroup.PreviewQuality.Default)
        {
          this.PreviewImage.Apply(true, true);
          this.PreviewImage.filterMode = FilterMode.Point;
        }
        else
        {
          this.PreviewImage.Apply(false);
          int num = Mathf.CeilToInt((previewQualityLevel == XUiC_WorldGenerationWindowGroup.PreviewQuality.Lowest ? 0.25f : (previewQualityLevel == XUiC_WorldGenerationWindowGroup.PreviewQuality.Low ? 0.5f : 0.5f)) * (float) this.WorldSize);
          Texture2D _targetTex = new Texture2D(num, num);
          this.PreviewImage.PointScaleNoAlloc(_targetTex);
          _targetTex.Apply(true, true);
          UnityEngine.Object.Destroy((UnityEngine.Object) this.PreviewImage);
          this.PreviewImage = _targetTex;
        }
      }
    }
  }

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public StreetTile GetStreetTileGrid(Vector2i pos) => this.GetStreetTileGrid(pos.x, pos.y);

  public StreetTile GetStreetTileGrid(int x, int y)
  {
    if ((long) (uint) x >= (long) this.StreetTileMapSize)
      return (StreetTile) null;
    return (long) (uint) y >= (long) this.StreetTileMapSize ? (StreetTile) null : this.StreetTileMap[x, y];
  }

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public StreetTile GetStreetTileWorld(Vector2i pos) => this.GetStreetTileWorld(pos.x, pos.y);

  public StreetTile GetStreetTileWorld(int x, int y)
  {
    x /= 150;
    if ((long) (uint) x >= (long) this.StreetTileMapSize)
      return (StreetTile) null;
    y /= 150;
    return (long) (uint) y >= (long) this.StreetTileMapSize ? (StreetTile) null : this.StreetTileMap[x, y];
  }

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public float GetHeight(Vector2 pos) => this.GetHeight((int) pos.x, (int) pos.y);

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public float GetHeight(Vector2i pos) => this.GetHeight(pos.x, pos.y);

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public float GetHeight(float x, float y) => this.GetHeight((int) x, (int) y);

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public float GetHeight(int x, int y)
  {
    return (long) (uint) x >= (long) this.WorldSize || (long) (uint) y >= (long) this.WorldSize ? 0.0f : this.HeightMap[x + y * this.WorldSize];
  }

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public void SetHeight(int index, float height) => this.HeightMap[index] = height;

  public void SetHeight(int x, int y, float height)
  {
    if ((long) (uint) x >= (long) this.WorldSize || (long) (uint) y >= (long) this.WorldSize)
      return;
    this.SetHeight(x + y * this.WorldSize, height);
  }

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public void SetHeightTrusted(int x, int y, float height)
  {
    this.SetHeight(x + y * this.WorldSize, height);
  }

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public TerrainType GetTerrainType(Vector2i pos) => this.GetTerrainType(pos.x, pos.y);

  public TerrainType GetTerrainType(int x, int y)
  {
    x /= 256 /*0x0100*/;
    if ((long) (uint) x >= (long) this.terrainTypeMap.data.GetLength(0))
      return TerrainType.none;
    y /= 256 /*0x0100*/;
    return (long) (uint) y >= (long) this.terrainTypeMap.data.GetLength(1) ? TerrainType.none : this.terrainTypeMap.data[x, y];
  }

  public BiomeType GetBiome(Vector2i pos) => this.GetBiome(pos.x, pos.y);

  public BiomeType GetBiome(int x, int y)
  {
    int index = x / 8 + y / 8 * this.BiomeSize;
    if ((long) (uint) index >= (long) (this.BiomeSize * this.BiomeSize))
      return BiomeType.forest;
    Color32 color32 = this.biomeDest[index];
    BiomeType biome = BiomeType.forest;
    if ((int) color32.g == (int) WorldBuilderConstants.burntForestCol.g)
      biome = BiomeType.burntForest;
    else if ((int) color32.g == (int) WorldBuilderConstants.desertCol.g)
      biome = BiomeType.desert;
    else if ((int) color32.g == (int) WorldBuilderConstants.snowCol.g)
      biome = BiomeType.snow;
    else if ((int) color32.g == (int) WorldBuilderConstants.wastelandCol.g)
      biome = BiomeType.wasteland;
    return biome;
  }

  public void SetWater(int x, int y, byte height)
  {
    if ((long) (uint) x >= (long) this.WorldSize || (long) (uint) y >= (long) this.WorldSize)
      return;
    this.WaterMap[x + y * this.WorldSize] = height;
  }

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public byte GetWater(int x, int y)
  {
    return (long) (uint) x >= (long) this.WorldSize || (long) (uint) y >= (long) this.WorldSize ? (byte) 0 : this.WaterMap[x + y * this.WorldSize];
  }

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public byte GetWater(int _index)
  {
    return (long) (uint) _index >= (long) (this.WorldSize * this.WorldSize) ? (byte) 0 : this.WaterMap[_index];
  }

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public byte GetRad(int x, int y)
  {
    return (long) (uint) x >= (long) this.WorldSize || (long) (uint) y >= (long) this.WorldSize ? (byte) 0 : this.radDest[x + y * this.WorldSize].r;
  }

  [MethodImpl(MethodImplOptions.AggressiveInlining)]
  public byte GetRad(int _index)
  {
    return (long) (uint) _index >= (long) (this.WorldSize * this.WorldSize) ? (byte) 0 : this.radDest[_index].r;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void serializePrefabs(Stream stream)
  {
    this.PrefabManager.SavePrefabData(stream);
    if (this.UsePreviewer)
      return;
    this.PrefabManager.UsedPrefabsWorld.Clear();
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void serializeRawHeightmap(Stream stream)
  {
    HeightMapUtils.SaveHeightMapRAW(stream, this.HeightMap, -1f);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public IEnumerator DrawRoads(Color32[] dest)
  {
    MicroStopwatch ms = new MicroStopwatch(true);
    byte[] ids = new byte[this.WorldSize * this.WorldSize];
    int i;
    for (i = 0; i < this.wildernessPaths.Count; ++i)
    {
      this.wildernessPaths[i].DrawPathToRoadIds(ids);
      if (this.IsMessageElapsed())
        yield return (object) this.SetMessage(string.Format(Localization.Get("xuiRwgDrawRoadsWilderness"), (object) (100 * i / this.wildernessPaths.Count)));
    }
    for (i = 0; i < this.highwayPaths.Count; ++i)
    {
      this.highwayPaths[i].DrawPathToRoadIds(ids);
      if (this.IsMessageElapsed())
        yield return (object) this.SetMessage(string.Format(Localization.Get("xuiRwgDrawRoadsProgress"), (object) (100 * i / this.highwayPaths.Count)));
    }
    this.PathShared.ConvertIdsToColors(ids, dest);
    Log.Out($"DrawRoads in {(ValueType) (float) ((double) ms.ElapsedMilliseconds * (1.0 / 1000.0))}");
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void serializePlayerSpawns(Stream stream)
  {
    using (StreamWriter streamWriter = new StreamWriter(stream, SdEncoding.UTF8NoBOM, 1024 /*0x0400*/, true))
    {
      streamWriter.WriteLine("<spawnpoints>");
      if (this.playerSpawns != null)
      {
        for (int index = 0; index < this.playerSpawns.Count; ++index)
        {
          WorldBuilder.PlayerSpawn playerSpawn = this.playerSpawns[index];
          streamWriter.WriteLine($"    <spawnpoint position=\"{(playerSpawn.Position.x - (float) this.WorldSize / 2f).ToCultureInvariantString()},{playerSpawn.Position.y.ToCultureInvariantString()},{(playerSpawn.Position.z - (float) this.WorldSize / 2f).ToCultureInvariantString()}\" rotation=\"0,{playerSpawn.Rotation},0\"/>");
        }
      }
      streamWriter.WriteLine("</spawnpoints>");
    }
  }

  public IEnumerator SetMessage(string _message, bool _logToConsole = false, bool _ignoreCancel = false)
  {
    if (_message != null)
      _message += $"\n{Localization.Get("xuiTime")} {this.totalMS.Elapsed.Minutes}:{this.totalMS.Elapsed.Seconds:00}";
    if (!GameManager.IsDedicatedServer)
    {
      if (!_ignoreCancel)
        this.IsCanceled |= this.CheckCancel();
      if (_message != null)
      {
        if (!_ignoreCancel && this.IsCanceled)
          _message = "Canceling...";
        if (!XUiC_ProgressWindow.IsWindowOpen())
          XUiC_ProgressWindow.Open(LocalPlayerUI.primaryUI, _message, _escClosable: false, _closeOpenWindows: false, _useShadow: true);
        else if (_message != this.setMessageLast)
        {
          this.setMessageLast = _message;
          XUiC_ProgressWindow.SetText(LocalPlayerUI.primaryUI, _message);
        }
      }
      else
      {
        this.setMessageLast = string.Empty;
        XUiC_ProgressWindow.Close(LocalPlayerUI.primaryUI);
      }
      yield return (object) this.endOfFrameHandle;
    }
    if (_logToConsole && _message != null)
      Log.Out("WorldGenerator:" + _message.Replace("\n", ": "));
    yield return (object) null;
  }

  public bool IsMessageElapsed()
  {
    if (this.messageMS.ElapsedMilliseconds <= 600L)
      return false;
    this.messageMS.ResetAndRestart();
    return true;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public bool CheckCancel()
  {
    return this.UsePreviewer && (bool) (OneAxisInputControl) PlatformManager.NativePlatform.Input.PrimaryPlayer.GUIActions.Cancel;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public Vector2i getRotatedPoint(int x, int y, int cx, int cy, int angle)
  {
    return new Vector2i(Mathf.RoundToInt((float) ((double) (x - cx) * Math.Cos((double) angle) - (double) (y - cy) * Math.Sin((double) angle)) + (float) cx), Mathf.RoundToInt((float) ((double) (x - cx) * Math.Sin((double) angle) + (double) (y - cy) * Math.Cos((double) angle)) + (float) cy));
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void GetTerrainProperties(
    string biomeTypeName,
    string terrainTypeName,
    string comboTypeName,
    out Vector2 _scaleMinMax,
    out int _clusterCount,
    out int _clusterRadius,
    out float _clusterStrength,
    out bool useBiomeMask,
    out float biomeCutoff)
  {
    _scaleMinMax = Vector2.one;
    string _input1 = this.thisWorldProperties.GetString(comboTypeName + ".scale");
    if (_input1 == string.Empty)
      _input1 = this.thisWorldProperties.GetString(terrainTypeName + ".scale");
    if (_input1 != string.Empty)
      _scaleMinMax = StringParsers.ParseVector2(_input1);
    _scaleMinMax *= 0.5f;
    _clusterCount = 3;
    _clusterRadius = 85;
    _clusterStrength = 1f;
    string _input2 = this.thisWorldProperties.GetString(comboTypeName + ".clusters");
    if (_input2 == string.Empty)
      _input2 = this.thisWorldProperties.GetString(terrainTypeName + ".clusters");
    if (_input2 != string.Empty)
    {
      Vector3 vector3 = StringParsers.ParseVector3(_input2);
      _clusterCount = (int) vector3.x;
      _clusterRadius = (int) (256.0 * (double) vector3.y);
      _clusterStrength = vector3.z;
    }
    useBiomeMask = false;
    string _input3 = this.thisWorldProperties.GetString(comboTypeName + ".use_biome_mask");
    if (_input3 == string.Empty)
      _input3 = this.thisWorldProperties.GetString(terrainTypeName + ".use_biome_mask");
    if (_input3 != string.Empty)
      useBiomeMask = StringParsers.ParseBool(_input3);
    biomeCutoff = 0.1f;
    string _input4 = this.thisWorldProperties.GetString(comboTypeName + ".biome_mask_min");
    if (_input4 == string.Empty)
      _input4 = this.thisWorldProperties.GetString(terrainTypeName + ".biome_mask_min");
    if (!(_input4 != string.Empty))
      return;
    biomeCutoff = StringParsers.ParseFloat(_input4);
  }

  public static string GetGeneratedWorldName(string _worldSeedName, int _worldSize = 8192 /*0x2000*/)
  {
    return RandomCountyNameGenerator.GetName(_worldSeedName.GetHashCode() + _worldSize);
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static int distanceSqr(Vector2i pointA, Vector2i pointB)
  {
    Vector2i vector2i = pointA - pointB;
    return vector2i.x * vector2i.x + vector2i.y * vector2i.y;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public static float distanceSqr(Vector2 pointA, Vector2 pointB)
  {
    Vector2 vector2 = pointA - pointB;
    return (float) ((double) vector2.x * (double) vector2.x + (double) vector2.y * (double) vector2.y);
  }

  public int GetCount(string _name, WorldBuilder.GenerationSelections _selection, GameRandom _rand = null)
  {
    float optionalValue1 = -1f;
    float optionalValue2 = 0.0f;
    float optionalValue3 = 0.0f;
    this.thisWorldProperties.ParseVec($"{_name}.count", ref optionalValue1, ref optionalValue2, ref optionalValue3);
    if ((double) optionalValue1 < 0.0)
      return -1;
    float num = optionalValue1;
    switch (_selection)
    {
      case WorldBuilder.GenerationSelections.Default:
        num = optionalValue2;
        break;
      case WorldBuilder.GenerationSelections.Many:
        num = optionalValue3;
        break;
    }
    int count = (int) num;
    if (_rand != null && (double) _rand.RandomFloat < (double) num - (double) count)
      ++count;
    return count;
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public void TestGenerateHeights()
  {
    for (int index1 = 0; index1 < this.WorldSize; ++index1)
    {
      float num = 0.0f;
      for (int index2 = 0; index2 < this.WorldSize; ++index2)
      {
        this.HeightMap[index2 + index1 * this.WorldSize] = num;
        if ((index2 & 3) == 3)
        {
          num += 2f;
          if ((double) num > (double) byte.MaxValue)
            num = 0.0f;
        }
      }
    }
  }

  [CompilerGenerated]
  [PublicizedFrom(EAccessModifier.Private)]
  public void \u003C\u002Ector\u003Eb__82_0(Stream stream)
  {
    stream.Write(ReadOnlySpan<byte>.op_Implicit(ImageConversion.EncodeArrayToPNG((Array) this.biomeDest, GraphicsFormat.R8G8B8A8_UNorm, (uint) this.BiomeSize, (uint) this.BiomeSize, (uint) (this.BiomeSize * 4))));
  }

  [CompilerGenerated]
  [PublicizedFrom(EAccessModifier.Private)]
  public void \u003C\u002Ector\u003Eb__82_1(Stream stream)
  {
    stream.Write(ReadOnlySpan<byte>.op_Implicit(ImageConversion.EncodeArrayToPNG((Array) this.radDest, GraphicsFormat.R8G8B8A8_UNorm, (uint) this.WorldSize, (uint) this.WorldSize, (uint) (this.WorldSize * 4))));
  }

  [CompilerGenerated]
  [PublicizedFrom(EAccessModifier.Private)]
  public void \u003C\u002Ector\u003Eb__82_2(Stream stream)
  {
    stream.Write(ReadOnlySpan<byte>.op_Implicit(ImageConversion.EncodeArrayToPNG((Array) this.roadDest, GraphicsFormat.R8G8B8A8_UNorm, (uint) this.WorldSize, (uint) this.WorldSize, (uint) (this.WorldSize * 4))));
  }

  [CompilerGenerated]
  [PublicizedFrom(EAccessModifier.Private)]
  public bool \u003CCalcPlayerSpawnTiles\u003Eb__94_0(StreetTile st)
  {
    return !st.OverlapsRadiation && !st.AllIsWater && st.Township == null && (st.District == null || st.District.name == "wilderness") && (this.ForestBiomeWeight == 0 || st.BiomeType == BiomeType.forest) && !st.Used;
  }

  [CompilerGenerated]
  [PublicizedFrom(EAccessModifier.Private)]
  public int \u003CCalcPlayerSpawnTiles\u003Eb__94_1(StreetTile _t1, StreetTile _t2)
  {
    return this.CalcClosestTraderDistance(_t1).CompareTo(this.CalcClosestTraderDistance(_t2));
  }

  [CompilerGenerated]
  [PublicizedFrom(EAccessModifier.Private)]
  public void \u003CwriteStampsToMaps\u003Eb__118_0()
  {
    MicroStopwatch microStopwatch = new MicroStopwatch(true);
    for (int index = 0; index < this.terrainDest.Length; ++index)
      this.SetHeight(index, this.terrainDest[index] * (float) byte.MaxValue);
    Log.Out("writeStampsToMaps terrain in {0}", (object) (float) ((double) microStopwatch.ElapsedMilliseconds * (1.0 / 1000.0)));
  }

  [CompilerGenerated]
  [PublicizedFrom(EAccessModifier.Private)]
  public void \u003CwriteStampsToMaps\u003Eb__118_1()
  {
    MicroStopwatch microStopwatch = new MicroStopwatch(true);
    this.StampManager.DrawStampGroup(this.biomeLayer, this.biomeDest, this.BiomeSize, 0.125f);
    this.biomeLayer.Stamps.Clear();
    Log.Out("writeStampsToMaps biome in {0}", (object) (float) ((double) microStopwatch.ElapsedMilliseconds * (1.0 / 1000.0)));
  }

  [CompilerGenerated]
  [PublicizedFrom(EAccessModifier.Private)]
  public void \u003CwriteStampsToMaps\u003Eb__118_2()
  {
    MicroStopwatch microStopwatch = new MicroStopwatch(true);
    this.StampManager.DrawStampGroup(this.radiationLayer, this.radDest, this.WorldSize);
    Log.Out("writeStampsToMaps rad in {0}", (object) (float) ((double) microStopwatch.ElapsedMilliseconds * (1.0 / 1000.0)));
    microStopwatch.ResetAndRestart();
    for (int index1 = 0; index1 < this.waterLayer.Stamps.Count; ++index1)
    {
      Stamp stamp = this.waterLayer.Stamps[index1];
      int num1 = Utils.FastMax(Mathf.FloorToInt(stamp.Area.min.x), 0);
      int num2 = Utils.FastMin(Mathf.FloorToInt(stamp.Area.max.x), this.WorldSize - 1);
      int num3 = Utils.FastMax(Mathf.FloorToInt(stamp.Area.min.y), 0);
      int num4 = Utils.FastMin(Mathf.FloorToInt(stamp.Area.max.y), this.WorldSize - 1);
      for (int index2 = num3; index2 <= num4; ++index2)
      {
        for (int index3 = num1; index3 <= num2; ++index3)
        {
          int index4 = index3 + index2 * this.WorldSize;
          float num5 = this.waterDest[index4] * (float) byte.MaxValue;
          if ((double) this.terrainWaterDest[index4] * (double) byte.MaxValue - 0.5 > (double) num5)
          {
            this.waterDest[index4] = 0.0f;
            num5 = 0.0f;
          }
          this.WaterMap[index4] = (byte) num5;
        }
      }
    }
    this.waterLayer.Stamps.Clear();
    Log.Out("writeStampsToMaps WaterMap in {0}", (object) (float) ((double) microStopwatch.ElapsedMilliseconds * (1.0 / 1000.0)));
  }

  [CompilerGenerated]
  [PublicizedFrom(EAccessModifier.Private)]
  public void \u003CGenerateTerrainLast\u003Eb__119_0()
  {
    this.StampManager.DrawStampGroup(this.lowerLayer, this.terrainDest, this.terrainWaterDest, this.WorldSize);
    this.StampManager.DrawStampGroup(this.terrainLayer, this.terrainDest, this.terrainWaterDest, this.WorldSize);
    for (int index = 0; index < this.terrainDest.Length; ++index)
      this.SetHeight(index, Utils.FastMax(this.terrainDest[index] * (float) byte.MaxValue, 2f));
  }

  [CompilerGenerated]
  [PublicizedFrom(EAccessModifier.Private)]
  public void \u003CGenerateTerrainLast\u003Eb__119_1()
  {
    this.StampManager.DrawWaterStampGroup(this.waterLayer, this.waterDest, this.WorldSize);
  }

  [CompilerGenerated]
  [PublicizedFrom(EAccessModifier.Private)]
  public void \u003CGenerateTerrainLast\u003Eb__119_2()
  {
    for (int index1 = 0; index1 < this.waterLayer.Stamps.Count; ++index1)
    {
      Stamp stamp = this.waterLayer.Stamps[index1];
      int num1 = Utils.FastMax(Mathf.FloorToInt(stamp.Area.min.x), 0);
      int num2 = Utils.FastMin(Mathf.FloorToInt(stamp.Area.max.x), this.WorldSize - 1);
      int num3 = Utils.FastMax(Mathf.FloorToInt(stamp.Area.min.y), 0);
      int num4 = Utils.FastMin(Mathf.FloorToInt(stamp.Area.max.y), this.WorldSize - 1);
      for (int index2 = num3; index2 <= num4; ++index2)
      {
        for (int index3 = num1; index3 <= num2; ++index3)
        {
          int index4 = index3 + index2 * this.WorldSize;
          float num5 = this.waterDest[index4] * (float) byte.MaxValue;
          if ((double) this.terrainDest[index4] * (double) byte.MaxValue - 0.5 > (double) num5)
          {
            this.waterDest[index4] = 0.0f;
            num5 = 0.0f;
          }
          this.WaterMap[index4] = (byte) num5;
        }
      }
    }
  }

  public enum BiomeLayout
  {
    CenterForest,
    CenterWasteland,
    Circle,
    Line,
  }

  [PublicizedFrom(EAccessModifier.Private)]
  public class BiomeTypeData
  {
    public BiomeType Type;
    public float Percent;
    public int TileCount;
    public Vector2i Center;

    public BiomeTypeData(BiomeType _type, float _percent, int _totalTiles)
    {
      this.Type = _type;
      this.Percent = _percent;
      this.TileCount = Mathf.FloorToInt(_percent * (float) _totalTiles);
      if ((double) this.Percent <= 0.0 || this.TileCount != 0)
        return;
      this.TileCount = 1;
    }
  }

  public class WildernessPathInfo
  {
    public Vector2i Position;
    public int PoiId;
    public float PathRadius;
    public BiomeType Biome;
    public int Connections;
    public Path Path;
    public float highwayDistance;
    public Vector2 highwayPoint;

    public WildernessPathInfo(
      Vector2i _startPos,
      int _id,
      float _pathRadius,
      BiomeType _biome,
      int _connections = 0)
    {
      this.Position = _startPos;
      this.PoiId = _id;
      this.PathRadius = _pathRadius;
      this.Biome = _biome;
      this.Connections = _connections;
    }
  }

  public enum GenerationSelections
  {
    None,
    Few,
    Default,
    Many,
  }

  public struct PlayerSpawn(Vector3 _position, float _yRotation)
  {
    [PublicizedFrom(EAccessModifier.Private)]
    public const int cSafeDist = 60;
    public Vector3 Position = _position;
    public float Rotation = _yRotation;

    public bool IsTooClose(Vector3 _position)
    {
      double num1 = (double) _position.x - (double) this.Position.x;
      float num2 = _position.z - this.Position.z;
      return num1 * num1 + (double) num2 * (double) num2 < 3600.0;
    }
  }
}
