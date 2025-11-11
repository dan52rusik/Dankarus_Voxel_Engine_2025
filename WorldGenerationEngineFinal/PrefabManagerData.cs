// Decompiled with JetBrains decompiler
// Type: WorldGenerationEngineFinal.PrefabManagerData
// Assembly: Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null
// MVID: AF8FE50B-9889-4084-9FCD-E241DDFED80F
// Assembly location: C:\Program Files (x86)\Steam\steamapps\common\7 Days To Die\7DaysToDie_Data\Managed\Assembly-CSharp.dll

using System;
using System.Collections;
using System.Collections.Generic;

#nullable disable
namespace WorldGenerationEngineFinal;

public class PrefabManagerData
{
  public readonly Dictionary<string, PrefabData> AllPrefabDatas = new Dictionary<string, PrefabData>();
  public List<PrefabData> prefabDataList = new List<PrefabData>();
  public readonly FastTags<TagGroup.Poi> PartsAndTilesTags = FastTags<TagGroup.Poi>.Parse("streettile,part");
  public readonly FastTags<TagGroup.Poi> WildernessTags = FastTags<TagGroup.Poi>.Parse("wilderness");
  public readonly FastTags<TagGroup.Poi> TraderTags = FastTags<TagGroup.Poi>.Parse("trader");
  [PublicizedFrom(EAccessModifier.Private)]
  public int previewSeed;

  public IEnumerator LoadPrefabs()
  {
    if (this.AllPrefabDatas.Count == 0)
    {
      MicroStopwatch ms = new MicroStopwatch(true);
      List<PathAbstractions.AbstractedLocation> prefabs = PathAbstractions.PrefabsSearchPaths.GetAvailablePathsList(_ignoreDuplicateNames: true);
      FastTags<TagGroup.Poi> filter = FastTags<TagGroup.Poi>.Parse("navonly,devonly,testonly,biomeonly");
      for (int i = 0; i < prefabs.Count; ++i)
      {
        PathAbstractions.AbstractedLocation _location = prefabs[i];
        int num = _location.Folder.LastIndexOf("/Prefabs/");
        if (num < 0 || !_location.Folder.Substring(num + 8, 5).EqualsCaseInsensitive("/test"))
        {
          PrefabData prefabData = PrefabData.LoadPrefabData(_location);
          try
          {
            if (prefabData != null)
            {
              if (!prefabData.Tags.Test_AnySet(filter))
              {
                if (!prefabData.Tags.IsEmpty)
                  this.AllPrefabDatas[_location.Name.ToLower()] = prefabData;
              }
            }
          }
          catch (Exception ex)
          {
            Log.Error("Could not load prefab data for " + _location.Name);
          }
          if (ms.ElapsedMilliseconds > 500L)
          {
            yield return (object) null;
            ms.ResetAndRestart();
          }
        }
      }
      Log.Out("LoadPrefabs {0} of {1} in {2}", (object) this.AllPrefabDatas.Count, (object) prefabs.Count, (object) (float) ((double) ms.ElapsedMilliseconds * (1.0 / 1000.0)));
    }
  }

  public void ShufflePrefabData(int _seed)
  {
    this.prefabDataList.Clear();
    this.AllPrefabDatas.CopyValuesTo<string, PrefabData>((IList<PrefabData>) this.prefabDataList);
    PrefabManager.Shuffle<PrefabData>(_seed, ref this.prefabDataList);
  }

  public void Cleanup()
  {
    this.AllPrefabDatas.Clear();
    this.prefabDataList.Clear();
  }

  public Prefab GetPreviewPrefabWithAnyTags(
    FastTags<TagGroup.Poi> _tags,
    int _townshipId,
    Vector2i size = default (Vector2i),
    bool useAnySizeSmaller = false)
  {
    Vector2i minSize = useAnySizeSmaller ? Vector2i.zero : size;
    List<PrefabData> all = this.prefabDataList.FindAll((Predicate<PrefabData>) ([PublicizedFrom(EAccessModifier.Internal)] (_pd) => !_pd.Tags.Test_AnySet(this.PartsAndTilesTags) && PrefabManager.isSizeValid(_pd, minSize, size) && _pd.Tags.Test_AnySet(_tags)));
    if (all.Count == 0)
      return (Prefab) null;
    PrefabManager.Shuffle<PrefabData>(this.previewSeed, ref all);
    Prefab prefabWithAnyTags = new Prefab();
    prefabWithAnyTags.Load(all[0].location);
    ++this.previewSeed;
    return prefabWithAnyTags;
  }
}
