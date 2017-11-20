//-----------------------------------------------------------------------------

include "include/poison";

//-----------------------------------------------------------------------------

function process_combat_event()

  if(GetMana(me) > 14)
    spellattack();
    if((GetHp(me) < (GetMaxHP(me) / 2)) and (RandomInt(100) <= 80))
      CastSpell(me, me, GHEAL_SPID);
    endif
    if(GetPoisonLevel(me) and (RandomInt(8) >= 5))
      CastSpell(me, me, CURE_SPID);
    endif
  endif

  if(((GetHp(me) * 100) / GetMaxHp(me)) < GetFleePoint())
    flee();
  endif
  
endfunction

//-----------------------------------------------------------------------------

function process_flee_event()

  if(GetMana(me) > 14)
    if((GetHp(me) < (GetMaxHP(me) / 2)) && (RandomInt(8) >= 3))
      CastSpell(me, me, GHEAL_SPID);
      return 0;
    endif
    if((GetPoisonLevel(me) >= 1) && (RandomInt(8) >= 3))
      CastSpell(me, me, CURE_SPID);
      return 0;
    endif
    spellattack();
  endif

  if(((GetHp(me) * 100) / GetMaxHp(me)) >= GetFleePoint())
    return 1;
  endif
  return 0;

endfunction

//-----------------------------------------------------------------------------

function SpellAttack()

  if(CheckLineOfSight(me,opponent) and Distance(me, opponent) <= 15)
    if(LastSpellTime < ReadGameClock())
      if(opponent != me)
        CastOffensiveSpell(me, opponent);
      endif
      
      var STime := GetSpellTime();
      LastSpellTime := ReadGameClock() + RandomInt(STime) + STime;
    endif
  endif
  
endfunction

//-----------------------------------------------------------------------------

function prepare_for_fight()

  if(!IsReflected(me) and RandomInt(3) == 2)
    CastSpell(me, me, REFLECT_SPID);
  endif
  DisableMainEvents();
  EnableCombatEvents();

endfunction

//-----------------------------------------------------------------------------

function post_combat()

  if((GetHp(me) < (GetMaxHP(me) / 2)) and (RandomInt(4) >= 2))
    CastSpell(me, me, GHEAL_SPID);
  endif
  if(GetPoisonLevel(me))
    CastSpell(me, me, CURE_SPID);
  endif
  DisableCombatEvents();
  EnableMainEvents();
  sleepms(50);

  //look_around();

endfunction

//-----------------------------------------------------------------------------
