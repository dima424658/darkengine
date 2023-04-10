// Initializing commands

EXTERN void r3_init_defaults(int trans_depth,int clip_planes,r3e_space space,int ctxt_cnt,int stride);

// simple init call that most users will use, that just uses all the default
EXTERN void r3_init(void);

EXTERN void r3_close(void);

// Sets a world coord system inherited by all subsequent context creations.  
// The default is the identity transform.  As well, the handedness bit is set
// in the transform.  If you have a current context.

EXTERN void r3_set_default_world(mxs_trans *w);

EXTERN mxs_trans *r3_get_default_world();

// Sets the default camera space that all new contexts inherit.  The default
// is R3_CLIPPING.

EXTERN void r3_set_default_space(r3e_space s);

EXTERN r3e_space r3_get_default_space();
