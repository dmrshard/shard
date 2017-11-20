//================================================================================
//--------------------------------------------------------------------------------
// Inn Check
//--------------------------------------------------------------------------------
//================================================================================

use uo;
use os;
use cfgfile;
use util;

//--------------------------------------------------------------------------------

const DEF_INN_Z_RANGE   := 10;

//--------------------------------------------------------------------------------

function Inncheck(who)

  var innCfg := ReadConfigFile(INN_CFG);
  var Rlm, Loc, Z;
  
  foreach elemName in GetConfigStringKeys(innCfg)

    Loc  := SplitWords(innCfg[elemName].Loc);
    Rlm  := innCfg[elemName].realm;
    Z    := innCfg[elemName].Z;

    if(!Rlm)
      Rlm := _DEFAULT_REALM;
    endif

    if(Z != error)
      Z := SplitWords(Z);
      if(Z.size() == 0)
        Z := error;
      elseif(Z.size() == 1)
        Z[2] := DEF_INN_Z_RANGE;
      endif
      Z[1] := CInt(Z[1]);
      Z[2] := CInt(Z[2]);
    endif
    
    if(who.realm == Rlm)
      if(who.x >= CInt(Loc[1]) and who.x <= CInt(Loc[3]) and who.y >= CInt(Loc[2]) and who.y <= CInt(Loc[4]))
        if(Z != error)
          if(who.z >= Z[1]-Z[2] and who.z <= Z[1]+Z[2])
            return 1;
          endif
        else
          return 1;
        endif
      endif
    endif

  endforeach

  return 0;

endfunction 

//--------------------------------------------------------------------------------
