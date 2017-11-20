//--------------------------------------------------------------------------------

use util;

//--------------------------------------------------------------------------------

include "include/teach";
include "include/attributes/skills";
include "include/attributes/npcs";
include "include/player/basic";
include "include/skills/misc";
include "include/checks/locchecks";
include "include/clock";

//--------------------------------------------------------------------------------

const MAX_GAIN_BASE_VALUE          := 5;
const SKMASTER_GAIN_MOD            := 0.15; // 15% wiecej gdy juz raz bylismy GMem w danym skillu

const NO_STAT_AWARD                := 0x01;
const TEACH_AWARD                  := 0x02;
const GAIN_RESTRICT                := 0x04;
const STAT_GAIN_MODIFIED           := 0x08;
const STAT_GAIN_ENHANCED           := 0x10;
const STAT_ALWAYS_GAIN             := 0x20;
const GAIN_HALF_RESTRICT           := 0x40;

const AWARD_BONUS_ACTIVE           := 1;
const AWARD_BONUS_PERC             := 1.4;

const HAPPY_HOUR_GAIN_MOD          := 1.25;

//================================================================================
//--------------------------------------------------------------------------------
// Attribute Award
//--------------------------------------------------------------------------------
//================================================================================

function AwardPoints(who, SkId, SkPoints, GainFlags := 0, GainValue := -1)

  if(who.isa(POLCLASS_NPC))
    return AwardNpcPoints(who, SkId, SkPoints);
  endif

  if(IsBlockedSkillForClass(who, SkId))
    return 0;
  endif

  if(IsGainBlockTimed(who))
    return 0;
  endif

  if(WasEquipingItem(who, 10))
    return 0;
  endif

  SkPoints := CInt(SkPoints);
  
  if(IsSkillMastered(who, SkId))
    SkPoints += CInt(CDbl(SkPoints) * SKMASTER_GAIN_MOD);
  endif

  if(GetObjProperty(who, "#AFight"))
    if(GetBaseSkill(who, SkId) >= 60)
      return 0;
    endif
    SetBitOn(GainFlags, NO_STAT_AWARD);
  endif

  if(!who.isA(POLCLASS_MOBILE) or SkId == error or !SkPoints)
    return 0;
  endif

  var elem := GetSkillConfigElem(SkId);

  if(!elem)
    return elem;
  endif

  if(!IsBitOn(GainFlags, TEACH_AWARD))
    // uczy? to nie ma gaina dla siebie.
    if(IsLearning(who))
      return 0;
    endif
  else
    // podczas uczenia nie ma gainow statow w ogole
    SetBitOn(GainFlags, NO_STAT_AWARD);
  endif
  
  SkId  := Cint(SkId);

  if(IsBitOn(GainFlags, GAIN_RESTRICT) and GainValue >= 0)
    SkPoints := GetRestrictedPoints(who, SkId, GainValue, SkPoints);
    
    // blokada na staty, jak ktos robi zbyt latwy koks na swoj poziom (wtedy SkPoints bedzie 0, gdy diff-15 < skill)
    if(!SkPoints and GainValue >= 0 and GainValue <= 40)
      SetBitOn(GainFlags, NO_STAT_AWARD);
    endif
  elseif(IsBitOn(GainFlags, GAIN_HALF_RESTRICT) and GainValue >= 0)
    SkPoints := GetHalfRestrictedPoints(who, SkId, GainValue, SkPoints);
  endif

  var AttrId := GetAttributeIdBySkillId(SkId);

  if(SkPoints > 0)
    ModifyGainByFactor(who, SkPoints, AttrId);
  endif

  if(SkPoints)
    AwardRawAttributePoints(who, AttrId, SkPoints, SkId);
    if(IsBitOn(GainFlags, GAIN_RESTRICT))
      TeachStuds(who, SkId, SkPoints, GainValue, Elem);
    endif
  endif

  if(!IsBitOn(GainFlags, NO_STAT_AWARD))
    var adv := elem.StrAdv;

    if(PrepareStatAdvance(adv, GainFlags, GainValue))
      if((RandomInt(1000) < (adv[1])) or IsBitOn(GainFlags, STAT_ALWAYS_GAIN))
        ModifyGainByFactor(who, adv[2], ATTRIBUTEID_STRENGTH);
        AwardRawAttributePoints(who, ATTRIBUTEID_STRENGTH, adv[2], SkId, GainFlags);
      endif
    endif

    adv := elem.DexAdv;

    if(PrepareStatAdvance(adv, GainFlags, GainValue))
      if((RandomInt(1000) < (adv[1])) or IsBitOn(GainFlags, STAT_ALWAYS_GAIN))
        ModifyGainByFactor(who, adv[2], ATTRIBUTEID_DEXTERITY);
        AwardRawAttributePoints(who, ATTRIBUTEID_DEXTERITY, adv[2], SkId, GainFlags);
      endif
    endif

    adv := elem.IntAdv;

    if(PrepareStatAdvance(adv, GainFlags, GainValue))
      if((RandomInt(1000) < (adv[1])) or IsBitOn(GainFlags, STAT_ALWAYS_GAIN))
        ModifyGainByFactor(who, adv[2], ATTRIBUTEID_INTELLIGENCE);
        AwardRawAttributePoints(who, ATTRIBUTEID_INTELLIGENCE, adv[2], SkId, GainFlags);
      endif
    endif
  endif
  
  return 1;

endfunction

//--------------------------------------------------------------------------------

function AwardDefaultPoints(who, skillid, Diff := error)

  var Points := GetDefaultPointsForDiff(who, skillid, Diff);

  return AwardPoints(who, skillid, Points);

endfunction

//--------------------------------------------------------------------------------

function ModAwardByBonus(Points)

  if(AWARD_BONUS_ACTIVE and Points)
    return max(1, CInt(Points* AWARD_BONUS_PERC));
  endif

  return Points;

endfunction

//--------------------------------------------------------------------------------

function IsGainBlockTimed(who)

  var Time := CInt(GetObjProperty(who, PROP_GAIN_BLOCK));

  if(Time and Time > ReadGameClock())
    return 1;
  endif 

  return 0;

endfunction

//--------------------------------------------------------------------------------

function GainBlockTimed(who, Time)

  SetObjProperty(who, PROP_GAIN_BLOCK, ReadGameClock() + Time);

endfunction

//--------------------------------------------------------------------------------

function PrepareStatAdvance(byref adv, GainFlags, GainValue)

  if(!adv)
    return 0;
  endif

  adv := SplitWords(adv);

  if(adv.size() != 2)
    return 0;
  endif

  adv[1] := Cdbl(adv[1])*10;
  adv[2] := RandomDiceRoll(adv[2]);

  if(IsBitOn(GainFlags, STAT_GAIN_MODIFIED))
    GainValue := GainValue * 0.01;
    adv[1] := adv[1] / 2;
    adv[2] := adv[2] / 2;
    adv[1] := CInt( adv[1] + (adv[1] * GainValue) );
    adv[2] := CInt( adv[2] + (adv[2] * GainValue) );
  elseif(IsBitOn(GainFlags, STAT_GAIN_ENHANCED))
    adv[1] := adv[1] * GainValue;
    adv[2] := adv[2] * GainValue;
  endif

  return 1;

endfunction

//--------------------------------------------------------------------------------

function AwardRawAttributePoints(who, byref AttrId, rawpoints, SkId, GainFlags := 0)

  rawpoints := CInt(rawpoints);

  if(who.isa(POLCLASS_NPC) or !who.isA(POLCLASS_MOBILE) or !rawpoints)
    return;
  endif

  if(IsSetAttrFlag(who, AttrId, ATTRF_BLOCKED))
    if(!who.cmdlevel and !IsStatAttribute(AttrId))
      var MaxVal := GetSkillBaseMaxValue(who, SkId);
      if(GetBaseSkillBaseValue(who, SkId) > MaxVal)
        SendAttrEvent(who, EVID_SK_SET, AttrId, MaxVal);
      endif
    endif

    return;
  endif

  if(AWARD_BONUS_ACTIVE and RawPoints)
    RawPoints := max(1, CInt(RawPoints*AWARD_BONUS_PERC));
  endif

  if (RawPoints < 1)
    return;
  endif
 

  if(IsStatAttribute(AttrId))
    var Cfg := ReadConfigFile(CLASSES_CFG);
    var cfgClass := Cfg[GetCharClass(who)];

    case(AttrId)
      ATTRIBUTEID_STRENGTH:      rawpoints := CInt(rawpoints * (CInt(cfgClass.StrGain) * 0.01));
      ATTRIBUTEID_DEXTERITY:     rawpoints := CInt(rawpoints * (CInt(cfgClass.DexGain) * 0.01));
      ATTRIBUTEID_INTELLIGENCE:  rawpoints := CInt(rawpoints * (CInt(cfgClass.IntGain) * 0.01));
    endcase

    SendAttrEvent(who,EVID_ST_AWARD,AttrId,RawPoints, IsBitOn(GainFlags, STAT_ALWAYS_GAIN));
  else
 
    if(GetGlobalProperty("#HH") and RawPoints)
      RawPoints := max(1, CInt(RawPoints*HAPPY_HOUR_GAIN_MOD));
    endif

    SendAttrEvent(who,EVID_SK_AWARD,AttrId,RawPoints);
  endif
  
  if (RawPoints < 1)
    return;
  endif

  CheckGainAttributeWatcher(who, AttrId, rawpoints);

endfunction

//--------------------------------------------------------------------------------

function IsStatAttribute(AttrId)

  if(AttrId == ATTRIBUTEID_STRENGTH or AttrId == ATTRIBUTEID_DEXTERITY or AttrId == ATTRIBUTEID_INTELLIGENCE)
    return 1;
  else
    return 0;
  endif

endfunction

//--------------------------------------------------------------------------------

function SetSkillMastered(who, SkillID)

  if(IsPlayer(who))
    if(!IsSkillMastered(who, SkillID))
      var AttrId := GetAttributeIdBySkillId(SkillID);
      SetObjProperty(who, PROP_SKMASTER + AttrId, ReadGameClock());
    endif
  endif

endfunction

//--------------------------------------------------------------------------------

function IsSkillMastered(who, SkillID)

  if(IsPlayer(who))
    var AttrId := GetAttributeIdBySkillId(SkillID);
    if(CInt(GetObjProperty(who, PROP_SKMASTER + AttrId)) > 0)
      return 1;
    endif
  endif

  return 0;

endfunction

//================================================================================
//--------------------------------------------------------------------------------
// Nutrition Skill & Stat Gain
//--------------------------------------------------------------------------------
//================================================================================

function ModifyGainByNutr(who, byref Gain, byref AttrId)

  var Perc := GetNutrValue(who, NVI_HUNGER);

  if(Perc >= NUTR_FULL_FACTOR)
    Perc := ((NUTR_MAX_VAL - NUTR_FULL_FACTOR) - (Perc - NUTR_FULL_FACTOR)) / 5;
  else
    Perc := Perc / 10;
  endif

  var aPerc := GetNutrValue(who, GetNviByAttrId(AttrId));

  if(aPerc >= NUTR_FULL_FACTOR)
    aPerc := ((NUTR_MAX_VAL - NUTR_FULL_FACTOR) - (aPerc - NUTR_FULL_FACTOR)) / 5;
  else
    aPerc := aPerc / 10;
  endif

  // 0% (86-100) do 17% (0-85)

  Perc := (Perc + aPerc);

  if(Perc > 20)
    Perc := 20;
  endif

  Gain := (Gain * 0.90) + (Gain * (Perc * 0.01));
  Gain := CInt(Gain);

endfunction

//funkcja to zmniejszania jelsi strefa
//-------------------------------------------------------------------------------
function ModifyGainByFactor(who, byref Gain, byref AttrId)
  
  /*
  __NONPVP__ DELETED
  if(IsInNonPvPArea(who.x,who.y,who.realm))
    Gain := CInt(Gain * 0.9);
  endif
  */

  ModifyGainByNutr(who, Gain, AttrId);

endfunction
//--------------------------------------------------------------------------------

function GetNviByAttrId(AttrId)

  var AttrCfg := ReadConfigFile(ATTRIBUTES_CFG);

  case(AttrCfg[AttrId].NutrId)
    NVIID_PROTEIN:  return NVI_PROTEIN;
    NVIID_SUGAR:    return NVI_SUGAR;
    NVIID_VITAMIN:  return NVI_VITAMIN;
    NVIID_HUNGER:   
    default:        return NVI_HUNGER;
  endcase

endfunction

//--------------------------------------------------------------------------------

function IsMobGainable(mob, attacker := 0)
  
  if(!mob.isa(POLCLASS_NPC))
    return 1;
  endif

  if(!CanGainOnNpcWithCag(mob, attacker))
    return 0;
  endif

  return 1;

endfunction

//--------------------------------------------------------------------------------
