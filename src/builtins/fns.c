#include "../core.h"
#include "../utils/hash.h"
#include "../utils/mut.h"
#include "../utils/talloc.h"
#include "../nfns.h"



void ud_rec(B** p, usz d, usz r, i32* pos, usz* sh) {
  if (d==r) {
    i32* rp;
    *(*p)++ = m_i32arrv(&rp, r);
    memcpy(rp, pos, 4*r);
  } else {
    usz c = sh[d];
    for (usz i = 0; i < c; i++) {
      pos[d] = i;
      ud_rec(p, d+1, r, pos, sh);
    }
  }
}
B ud_c1(B t, B x) {
  if (isAtm(x)) {
    usz xu = o2s(x);
    if (LIKELY(xu<=I8_MAX)) {
      if (RARE(xu==0)) return emptyIVec();
      i8* rp; B r = m_i8arrv(&rp, xu);
      for (usz i = 0; i < xu; i++) rp[i] = i;
      return r;
    }
    if (xu<=I16_MAX) {
      i16* rp; B r = m_i16arrv(&rp, xu);
      for (usz i = 0; i < xu; i++) rp[i] = i;
      return r;
    }
    if (xu<=I32_MAX) {
      i32* rp; B r = m_i32arrv(&rp, xu);
      for (usz i = 0; i < xu; i++) rp[i] = i;
      return r;
    }
    f64* rp; B r = m_f64arrv(&rp, xu);
    for (usz i = 0; i < xu; i++) rp[i] = i;
    return r;
  }
  SGetU(x)
  usz xia = IA(x);
  if (RNK(x)!=1) thrF("↕: Argument must be either an integer or integer list (had rank %i)", RNK(x));
  if (xia>UR_MAX) thrF("↕: Result rank too large (%s≡≠𝕩)", xia);
  usz sh[xia];
  usz ria = 1;
  for (usz i = 0; i < xia; i++) {
    usz c = o2s(GetU(x, i));
    if (c > I32_MAX) thrM("↕: Result too large");
    sh[i] = c;
    if (c*(u64)ria >= U32_MAX) thrM("↕: Result too large");
    ria*= c;
  }
  decG(x);
  
  Arr* r = m_fillarrp(ria); fillarr_setFill(r, m_f64(0));
  B* rp = fillarr_ptr(r);
  for (usz i = 0; i < ria; i++) rp[i] = m_f64(0); // don't break if allocation errors
  
  usz* rsh = arr_shAlloc(r, xia);
  if (rsh) shcpy(rsh, sh, xia);
  
  i32 pos[xia]; B* crp = rp;
  ud_rec(&crp, 0, xia, pos, sh);
  
  if (ria) fillarr_setFill(r, inc(rp[0]));
  else {
    i32* fp;
    fillarr_setFill(r, m_i32arrv(&fp, xia));
    for (usz i = 0; i < xia; i++) fp[i] = 0;
  }
  return taga(r);
}

extern B rt_ud;
B ud_c2(B t, B w, B x) {
  return c2(rt_ud, w, x);
}

B ltack_c1(B t,      B x) {         return x; }
B ltack_c2(B t, B w, B x) { dec(x); return w; }
B rtack_c1(B t,      B x) {         return x; }
B rtack_c2(B t, B w, B x) { dec(w); return x; }

B fne_c1(B t, B x) {
  if (isAtm(x)) {
    dec(x);
    return emptyIVec();
  }
  ur xr = RNK(x);
  usz* sh = SH(x);
  usz or = 0;
  for (i32 i = 0; i < xr; i++) or|= sh[i];
  B r;
  if      (or<=I8_MAX ) { i8*  rp; r = m_i8arrv (&rp, xr); for (i32 i = 0; i < xr; i++) rp[i] = sh[i]; }
  else if (or<=I16_MAX) { i16* rp; r = m_i16arrv(&rp, xr); for (i32 i = 0; i < xr; i++) rp[i] = sh[i]; }
  else if (or<=I32_MAX) { i32* rp; r = m_i32arrv(&rp, xr); for (i32 i = 0; i < xr; i++) rp[i] = sh[i]; }
  else                  { f64* rp; r = m_f64arrv(&rp, xr); for (i32 i = 0; i < xr; i++) rp[i] = sh[i]; }
  decG(x); return r;
}
B feq_c1(B t, B x) {
  u64 r = depth(x);
  dec(x);
  return m_f64(r);
}


B feq_c2(B t, B w, B x) {
  bool r = equal(w, x);
  dec(w); dec(x);
  return m_i32(r);
}
B fne_c2(B t, B w, B x) {
  bool r = !equal(w, x);
  dec(w); dec(x);
  return m_i32(r);
}


extern B rt_indexOf;
B indexOf_c1(B t, B x) {
  if (isAtm(x)) thrM("⊐: 𝕩 cannot have rank 0");
  usz xia = IA(x);
  if (xia==0) { decG(x); return emptyIVec(); }

  u8 xe = TI(x,elType);
  #define LOOKUP(T) \
    usz tn = 1<<T, n = xia;                      \
    u##T* xp = (u##T*)i##T##any_ptr(x);          \
    i32* rp; B r = m_i32arrv(&rp, n);            \
    TALLOC(i32, tab, tn);                        \
    for (usz j=0; j<tn; j++) tab[j]=n;           \
    i32 u=0;                                     \
    for (usz i=0; i<n;  i++) {                   \
      u##T j=xp[i]; i32 t=tab[j];                \
      if (t==n) rp[i]=tab[j]=u++; else rp[i]=t;  \
    }                                            \
    decG(x); TFREE(tab);                         \
    return r
  if (RNK(x)==1 && xia>=16 && xe==el_i8 && xia<=(usz)I32_MAX+1) { LOOKUP(8); }
  if (RNK(x)==1 && xia>=256 && xe==el_i16 && xia<=(usz)I32_MAX+1) { LOOKUP(16); }
  #undef LOOKUP

  if (RNK(x)==1 && xe==el_i32) {
    i32* xp = i32any_ptr(x);
    i32 min=I32_MAX, max=I32_MIN;
    for (usz i = 0; i < xia; i++) {
      i32 c = xp[i];
      if (c<min) min = c;
      if (c>max) max = c;
    }
    i64 dst = 1 + (max-(i64)min);
    if ((dst<xia*5 || dst<50) && min!=I32_MIN) {
      i32* rp; B r = m_i32arrv(&rp, xia);
      TALLOC(i32, tmp, dst);
      for (i64 i = 0; i < dst; i++) tmp[i] = I32_MIN;
      i32* tc = tmp-min;
      i32 ctr = 0;
      for (usz i = 0; i < xia; i++) {
        i32 c = xp[i];
        if (tc[c]==I32_MIN) tc[c] = ctr++;
        rp[i] = tc[c];
      }
      decG(x); TFREE(tmp);
      return r;
    }
  }
  // if (RNK(x)==1) { // relies on equal hashes implying equal objects, which has like a 2⋆¯64 chance of being false per item
  //   // u64 s = nsTime();
  //   i32* rp; B r = m_i32arrv(&rp, xia);
  //   u64 size = xia*2;
  //   wyhashmap_t idx[size];
  //   i32 val[size];
  //   for (i64 i = 0; i < size; i++) { idx[i] = 0; val[i] = -1; }
  //   SGet(x)
  //   i32 ctr = 0;
  //   for (usz i = 0; i < xia; i++) {
  //     u64 hash = bqn_hash(Get(x,i), wy_secret);
  //     u64 p = wyhashmap(idx, size, &hash, 8, true, wy_secret);
  //     if (val[p]==-1) val[p] = ctr++;
  //     rp[i] = val[p];
  //   }
  //   dec(x);
  //   // u64 e = nsTime(); q1+= e-s;
  //   return r;
  // }
  if (RNK(x)==1) {
    // u64 s = nsTime();
    i32* rp; B r = m_i32arrv(&rp, xia);
    H_b2i* map = m_b2i(64);
    SGetU(x)
    i32 ctr = 0;
    for (usz i = 0; i < xia; i++) {
      bool had; u64 p = mk_b2i(&map, GetU(x,i), &had);
      if (had) rp[i] = map->a[p].val;
      else     rp[i] = map->a[p].val = ctr++;
    }
    free_b2i(map); decG(x);
    // u64 e = nsTime(); q1+= e-s;
    return r;
  }
  return c1(rt_indexOf, x);
}
B indexOf_c2(B t, B w, B x) {
  if (!isArr(w) || RNK(w)==0) thrM("⊐: 𝕨 must have rank at least 1");
  if (RNK(w)==1) {
    if (!isArr(x) || RNK(x)==0) {
      usz wia = IA(w);
      B el = isArr(x)? IGetU(x,0) : x;
      i32 res = wia;
      if (TI(w,elType)==el_i32) {
        if (q_i32(el)) {
          i32* wp = i32any_ptr(w);
          i32 v = o2iu(el);
          for (usz i = 0; i < wia; i++) {
            if (wp[i] == v) { res = i; break; }
          }
        }
      } else {
        SGetU(w)
        for (usz i = 0; i < wia; i++) {
          if (equal(GetU(w,i), el)) { res = i; break; }
        }
      }
      decG(w); dec(x);
      i32* rp; Arr* r = m_i32arrp(&rp, 1);
      arr_shAlloc(r, 0);
      rp[0] = res;
      return taga(r);
    } else {
      usz wia = IA(w);
      usz xia = IA(x);
      // TODO O(wia×xia) for small wia or xia
      i32* rp; B r = m_i32arrc(&rp, x);
      H_b2i* map = m_b2i(64);
      SGetU(x)
      SGetU(w)
      for (usz i = 0; i < wia; i++) {
        bool had; u64 p = mk_b2i(&map, GetU(w,i), &had);
        if (!had) map->a[p].val = i;
      }
      for (usz i = 0; i < xia; i++) rp[i] = getD_b2i(map, GetU(x,i), wia);
      free_b2i(map); decG(w); decG(x);
      return wia<=I8_MAX? taga(cpyI8Arr(r)) : wia<=I16_MAX? taga(cpyI16Arr(r)) : r;
    }
  }
  return c2(rt_indexOf, w, x);
}

B enclosed_0;
B enclosed_1;
extern B rt_memberOf;
B memberOf_c1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("∊: Argument cannot have rank 0");
  if (RNK(x)!=1) x = toCells(x);
  usz xia = IA(x);
  u8 xe = TI(x,elType);

  #define LOOKUP(T) \
    usz tn = 1<<T, n = xia;                                            \
    u##T* xp = (u##T*)i##T##any_ptr(x);                                \
    i8* rp; B r = m_i8arrv(&rp, n);                                    \
    TALLOC(u8, tab, tn);                                               \
    for (usz j=0; j<tn; j++) tab[j]=1;                                 \
    for (usz i=0; i<n;  i++) { u##T j=xp[i]; rp[i]=tab[j]; tab[j]=0; } \
    decG(x); TFREE(tab);                                                        \
    return num_squeeze(r)
  if (xia>=16 && xe==el_i8) { LOOKUP(8); }
  if (xia>=256 && xe==el_i16) { LOOKUP(16); }
  #undef LOOKUP
  // Radix-assisted lookup
  if (xia>=256 && xe==el_i32) {
    usz rx = 256, tn = 1<<16; // Radix; table length
    usz n = xia;
    u32* v0 = (u32*)i32any_ptr(x);
    i8* r0; B r = m_i8arrv(&r0, n);

    TALLOC(u8, alloc, 9*n+(tn+2*rx*sizeof(usz)));
    // Allocations                    count radix hash deradix
    usz *c0 = (usz*)(alloc);   // rx    X-----------------X
    usz *c1 = (usz*)(c0+rx);   // rx    X-----------------X
    u8  *k0 = (u8 *)(c1+rx);   //  n    X-----------------X
    u8  *k1 = (u8 *)(k0+n);    //            --/----------X
    u32 *v1 = (u32*)(k1);      //  n         X-X
    u32 *v2 = (u32*)(v1+n);    //  n           X----------X
    u8  *r2 = (u8 *)(k1+n);
    u8  *r1 = (u8 *)(r2+n);
    u8  *tab= (u8 *)(v2+n);    // tn

    // Count keys
    for (usz j=0; j<2*rx; j++) c0[j] = 0;
    for (usz i=0; i<n; i++) { u32 v=v0[i]; c0[(u8)(v>>24)]++; c1[(u8)(v>>16)]++; }
    // Exclusive prefix sum
    usz s0=0, s1=0;
    for (usz j=0; j<rx; j++) {
      usz p0 = s0,  p1 = s1;
      s0 += c0[j];  s1 += c1[j];
      c0[j] = p0;   c1[j] = p1;
    }
    // Radix moves
    for (usz i=0; i<n; i++) { u32 v=v0[i]; u8 k=k0[i]=(u8)(v>>24); usz c=c0[k]++; v1[c]=v; }
    for (usz i=0; i<n; i++) { u32 v=v1[i]; u8 k=k1[i]=(u8)(v>>16); usz c=c1[k]++; v2[c]=v; }
    // Table lookup
    for (usz j=0; j<tn; j++) tab[j]=1;
    u32 t0=v2[0]>>16; usz e=0;
    for (usz i=0; i<n; i++) {
      u32 v=v2[i], tv=v>>16;
      // Clear table when top bytes change
      if (RARE(tv!=t0)) { for (; e<i; e++) tab[(u16)v2[e]]=1; t0=tv; }
      u32 j=(u16)v; r2[i]=tab[j]; tab[j]=0;
    }
    // Radix unmoves
    memmove(c0+1, c0, (2*rx-1)*sizeof(usz)); c0[0]=c1[0]=0;
    for (usz i=0; i<n; i++) { r1[i]=r2[c1[k1[i]]++]; }
    for (usz i=0; i<n; i++) { r0[i]=r1[c0[k0[i]]++]; }
    decG(x); TFREE(alloc);
    return num_squeeze(r);
  }
  
  u64* rp; B r = m_bitarrv(&rp, xia);
  H_Sb* set = m_Sb(64);
  SGetU(x)
  for (usz i = 0; i < xia; i++) bitp_set(rp, i, !ins_Sb(&set, GetU(x,i)));
  free_Sb(set); decG(x);
  return r;
}
B memberOf_c2(B t, B w, B x) {
  if (isAtm(x) || RNK(x)!=1) goto bad;
  if (isAtm(w)) goto single;
  ur wr = RNK(w);
  if (wr==0) {
    B w0 = IGet(w, 0);
    dec(w);
    w = w0;
    goto single;
  }
  if (wr==1) goto many;
  goto bad;
  
  bad: return c2(rt_memberOf, w, x);
  
  B r;
  single: {
    usz xia = IA(x);
    SGetU(x)
    for (usz i = 0; i < xia; i++) if (equal(GetU(x, i), w)) { r = inc(enclosed_1); goto dec_wx; }
    r = inc(enclosed_0);
    dec_wx:; dec(w);
    goto dec_x;
  }
  
  
  many: {
    usz xia = IA(x);
    usz wia = IA(w);
    // TODO O(wia×xia) for small wia or xia
    H_Sb* set = m_Sb(64);
    SGetU(x) SGetU(w)
    bool had;
    for (usz i = 0; i < xia; i++) mk_Sb(&set, GetU(x,i), &had);
    u64* rp; r = m_bitarrv(&rp, wia);
    for (usz i = 0; i < wia; i++) bitp_set(rp, i, has_Sb(set, GetU(w,i)));
    free_Sb(set); decG(w);
    goto dec_x;
  }
  
  dec_x:;
  decG(x);
  return r;
}

extern B rt_find;
B find_c1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("⍷: Argument cannot have rank 0");
  usz xia = IA(x);
  B xf = getFillQ(x);
  if (RNK(x)!=1) return c1(rt_find, x);
  
  B r = emptyHVec();
  H_Sb* set = m_Sb(64);
  SGetU(x)
  for (usz i = 0; i < xia; i++) {
    B c = GetU(x,i);
    if (!ins_Sb(&set, c)) r = vec_add(r, inc(c));
  }
  free_Sb(set); decG(x);
  return withFill(r, xf);
}
B find_c2(B t, B w, B x) {
  return c2(rt_find, w, x);
}

extern B rt_count;
B count_c1(B t, B x) {
  if (isAtm(x) || RNK(x)==0) thrM("⊒: Argument cannot have rank 0");
  if (RNK(x)>1) x = toCells(x);
  usz xia = IA(x);
  u8 xe = TI(x,elType);

  #define LOOKUP(T) \
    usz tn = 1<<T, n = xia;                      \
    u##T* xp = (u##T*)i##T##any_ptr(x);          \
    i32* rp; B r = m_i32arrv(&rp, n);            \
    TALLOC(i32, tab, tn);                        \
    for (usz j=0; j<tn; j++) tab[j]=0;           \
    for (usz i=0; i<n;  i++) rp[i]=tab[xp[i]]++; \
    decG(x); TFREE(tab);                                                        \
    return r
  if (xia>=16 && xe==el_i8 && xia<=(usz)I32_MAX+1) { LOOKUP(8); }
  if (xia>=256 && xe==el_i16 && xia<=(usz)I32_MAX+1) { LOOKUP(16); }
  #undef LOOKUP

  i32* rp; B r = m_i32arrv(&rp, xia);
  H_b2i* map = m_b2i(64);
  SGetU(x)
  for (usz i = 0; i < xia; i++) {
    bool had; u64 p = mk_b2i(&map, GetU(x,i), &had);
    rp[i] = had? ++map->a[p].val : (map->a[p].val = 0);
  }
  decG(x); free_b2i(map);
  return r;
}
B count_c2(B t, B w, B x) {
  return c2(rt_count, w, x);
}

static H_b2i* prevImports;
i32 getPrevImport(B path) { // -1 for unset, -2 for unfinished
  if (prevImports==NULL) prevImports = m_b2i(16);
  
  bool had; i32 prev = mk_b2i(&prevImports, path, &had);
  if (had && prevImports->a[prev].val!=-1) return prevImports->a[prev].val;
  prevImports->a[prev].val = -2;
  return -1;
}
void setPrevImport(B path, i32 pos) {
  bool had; i32 prev = mk_b2i(&prevImports, path, &had);
  prevImports->a[prev].val = pos;
}
void clearImportCacheMap() {
  if (prevImports!=NULL) free_b2i(prevImports);
  prevImports = NULL;
}

static H_b2i* globalNames;
static B globalNameList;
i32 str2gid(B s) {
  if (globalNames==NULL) {
    globalNames = m_b2i(32);
    globalNameList = emptyHVec();
  }
  bool had;
  u64 p = mk_b2i(&globalNames, s, &had);
  // if(had) print_fmt("str2gid %R → %i\n", s, globalNames->a[p].val); else print_fmt("str2gid %R → %i!!\n", s, IA(globalNameList));
  if(had) return globalNames->a[p].val;
  
  i32 r = IA(globalNameList);
  globalNameList = vec_addN(globalNameList, inc(s));
  globalNames->a[p].val = r;
  return r;
}

i32 str2gidQ(B s) { // if the name doesn't exist yet, return -1
  if (globalNames==NULL) return -1; // if there are no names, there certainly won't be the queried one
  return getD_b2i(globalNames, s, -1);
}

B gid2str(i32 n) {
  B r = IGetU(globalNameList, n);
  // print_fmt("gid2str %i → %R\n", n, r);
  return r;
}

void* profiler_makeMap() {
  return m_b2i(64);
}
i32 profiler_index(void** mapRaw, B comp) {
  H_b2i* map = *(H_b2i**)mapRaw;
  i32 r;
  bool had; u64 p = mk_b2i(&map, comp, &had);
  if (had) r = map->a[p].val;
  else     r = map->a[p].val = map->pop-1;
  *(H_b2i**)mapRaw = map;
  return r;
}
void profiler_freeMap(void* mapRaw) {
  free_b2i((H_b2i*)mapRaw);
}



void fun_gcFn() {
  if (prevImports!=NULL) mm_visitP(prevImports);
  if (globalNames!=NULL) mm_visitP(globalNames);
  mm_visit(enclosed_0);
  mm_visit(enclosed_1);
  mm_visit(globalNameList);
}



static void print_funBI(FILE* f, B x) { fprintf(f, "%s", pfn_repr(c(Fun,x)->extra)); }
static B funBI_uc1(B t, B o,      B x) { return c(BFn,t)->uc1(t, o,    x); }
static B funBI_ucw(B t, B o, B w, B x) { return c(BFn,t)->ucw(t, o, w, x); }
static B funBI_im(B t, B x) { return c(BFn,t)->im(t, x); }
static B funBI_identity(B x) { return inc(c(BFn,x)->ident); }
void fns_init() {
  gc_addFn(fun_gcFn);
  TIi(t_funBI,print) = print_funBI;
  TIi(t_funBI,identity) = funBI_identity;
  TIi(t_funBI,fn_uc1) = funBI_uc1;
  TIi(t_funBI,fn_ucw) = funBI_ucw;
  TIi(t_funBI,fn_im) = funBI_im;
  { u64* p; Arr* a=m_bitarrp(&p, 1); arr_shAlloc(a,0); *p= 0;    enclosed_0=taga(a); }
  { u64* p; Arr* a=m_bitarrp(&p, 1); arr_shAlloc(a,0); *p=~0ULL; enclosed_1=taga(a); }
}
