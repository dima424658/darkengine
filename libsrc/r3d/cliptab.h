typedef struct {
   void (*clip_polygon)(int n, r3s_phandle *src, r3s_phandle **dest);
   void (*clip_line)   (r3s_phandle *src, r3s_phandle **dest);
} r3s_clip_tab;
