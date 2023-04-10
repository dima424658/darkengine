
enum {
  TMC_ADD_PROCESS,
  TMC_REM_PROCESS,
  TMC_ACT_PROCESS,
  TMC_DEACT_PROCESS,
  TMC_SET_PROC_DENOM,
  TMC_SET_PROC_FREQ,
  TMC_SET_PROC_PER,
  TMC_TIMER_FNS
};

extern  pfv *tm_ftab;  

extern  pfv tm_ftab_libt [TMC_TIMER_FNS]; 
extern  pfv tm_ftab_ail [TMC_TIMER_FNS];  

