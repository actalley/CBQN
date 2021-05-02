#include "h.h"

typedef struct FillArr {
  struct Arr;
  B fill;
  B a[];
} FillArr;

B asFill(B x) { // consumes
  if (isArr(x)) {
    HArr_p r = m_harrUc(x);
    usz ia = r.c->ia;
    BS2B xget = TI(x).get;
    bool noFill = false;
    for (usz i = 0; i < ia; i++) if ((r.a[i]=asFill(xget(x,i))).u == bi_noFill.u) noFill = true;
    dec(x);
    if (noFill) { ptr_dec(r.c); return bi_noFill; }
    return r.b;
  }
  if (isF64(x)|isI32(x)) return m_i32(0);
  if (isC32(x)) return m_c32(' ');
  dec(x);
  return bi_noFill;
}
B getFill(B x) { // consumes; can return bi_noFill
  bool defZero = true;
  #ifdef CATCH_ERRORS
  defZero = false;
  #endif
  if (isArr(x)) {
    u8 t = v(x)->type;
    if (t==t_fillarr  ) { B r = inc(c(FillArr,x            )->fill); dec(x); return r; }
    if (t==t_fillslice) { B r = inc(c(FillArr,c(Slice,x)->p)->fill); dec(x); return r; }
    if (t==t_c32arr || t==t_c32slice) { dec(x); return m_c32(' '); }
    if (t==t_i32arr || t==t_i32slice) { dec(x); return m_f64(0  ); }
    if (t==t_f64arr || t==t_f64slice) { dec(x); return m_f64(0  ); }
    dec(x);
    return defZero? m_f64(0) : bi_noFill;
  }
  if (isF64(x)|isI32(x)) return m_i32(0);
  if (isC32(x)) return m_c32(' ');
  dec(x);
  return defZero? m_f64(0) : bi_noFill;
}
B getFillE(B x) { // errors if there's no fill
  B xf = getFill(x);
  if (noFill(xf)) {
    if (PROPER_FILLS) thrM("No fill found");
    else return m_f64(0);
  }
  return xf;
}
bool noFill(B x) { return x.u == bi_noFill.u; }

typedef struct FillSlice {
  struct Slice;
  B* a;
} FillSlice;
B m_fillslice(B p, B* ptr) {
  FillSlice* r = mm_allocN(sizeof(FillSlice), t_fillslice);
  r->p = p;
  r->a = ptr;
  return tag(r, ARR_TAG);
}

B* fillarr_ptr(B x) { VT(x,t_fillarr); return c(FillArr,x)->a; }
B fillarr_slice  (B x, usz s) {return m_fillslice(x                 , c(FillArr  ,x)->a+s); }
B fillslice_slice(B x, usz s) { B r = m_fillslice(inc(c(Slice,x)->p), c(FillSlice,x)->a+s); dec(x); return r; }


B fillarr_get   (B x, usz n) { VT(x,t_fillarr  ); return inc(c(FillArr  ,x)->a[n]); }
B fillslice_get (B x, usz n) { VT(x,t_fillslice); return inc(c(FillSlice,x)->a[n]); }
B fillarr_getU  (B x, usz n) { VT(x,t_fillarr  ); return     c(FillArr  ,x)->a[n] ; }
B fillslice_getU(B x, usz n) { VT(x,t_fillslice); return     c(FillSlice,x)->a[n] ; }
void fillarr_free(B x) {
  decSh(x);
  B* p = c(FillArr, x)->a;
  dec(c(FillArr, x)->fill);
  usz ia = a(x)->ia;
  for (usz i = 0; i < ia; i++) dec(p[i]);
}
void fillarr_visit(B x) {
  usz ia = a(x)->ia; B* p = fillarr_ptr(x);
  mm_visit(c(FillArr,x)->fill);
  for (usz i = 0; i < ia; i++) mm_visit(p[i]);
}
bool fillarr_canStore(B x) { return true; }

static inline void fillarr_init() {
  ti[t_fillarr].get   = fillarr_get;   ti[t_fillslice].get   = fillslice_get;
  ti[t_fillarr].getU  = fillarr_getU;  ti[t_fillslice].getU  = fillslice_getU;
  ti[t_fillarr].slice = fillarr_slice; ti[t_fillslice].slice = fillslice_slice;
  ti[t_fillarr].free  = fillarr_free;  ti[t_fillslice].free  =     slice_free;
  ti[t_fillarr].visit = fillarr_visit; ti[t_fillslice].visit =     slice_visit;
  ti[t_fillarr].print =     arr_print; ti[t_fillslice].print = arr_print;
  ti[t_fillarr].isArr = true;          ti[t_fillslice].isArr = true;
  ti[t_fillarr].canStore = fillarr_canStore;
}

B m_unit(B x) {
  B xf = asFill(inc(x));
  if (noFill(xf)) {
    HArr_p r = m_harrUp(1);
    arr_shAllocR(r.b, 0);
    r.a[0] = x;
    return r.b;
  }
  B r = m_arr(fsizeof(FillArr,a,B,1), t_fillarr);
  arr_shAllocI(r, 1, 0);
  c(FillArr,r)->fill = xf;
  c(FillArr,r)->a[0] = x;
  return r;
}
B m_atomUnit(B x) {
  if (isNum(x)) {
    B r;
    if (q_i32(x)) { r=m_i32arrp(1); i32arr_ptr(r)[0] = o2iu(x); }
    else          { r=m_f64arrp(1); f64arr_ptr(r)[0] = o2fu(x); }
    arr_shAllocR(r,0);
    return r;
  }
  if (isC32(x)) {
    B r = m_c32arrp(1);
    c32arr_ptr(r)[0] = o2cu(x);
    arr_shAllocR(r,0);
    return r;
  }
  return m_unit(x);
}

void validateFill(B x) {
  if (isArr(x)) {
    BS2B xgetU = TI(x).getU;
    usz ia = a(x)->ia;
    for (usz i = 0; i < ia; i++) validateFill(xgetU(x,i));
  } else if (isF64(x)) {
    assert(x.f==0);
  } else if (isC32(x)) {
    assert(' '==(u32)x.u);
  }
}

B fill_both(B w, B x) { // doesn't consume
  B fw = getFill(inc(w));
  if (noFill(fw)) return bi_noFill;
  B fx = getFill(inc(x));
  if (!equal(fw, fx)) {
    dec(fw); dec(fx);
    return bi_noFill;
  }
  dec(fw);
  return fx;
}

B withFill(B x, B fill) { // consumes both
  assert(isArr(x));
  #ifdef DEBUG
  validateFill(fill);
  #endif
  if (noFill(fill) && v(x)->type!=t_fillarr && v(x)->type!=t_fillslice) return x;
  switch(v(x)->type) {
    case t_f64arr : case t_f64slice:
    case t_i32arr : case t_i32slice: if(fill.u == m_i32(0  ).u) return x; break;
    case t_c32arr : case t_c32slice: if(fill.u == m_c32(' ').u) return x; break;
    case t_fillslice: if (equal(c(FillArr,c(Slice,x)->p)->fill, fill)) { dec(fill); return x; } break;
    case t_fillarr: if (equal(c(FillArr,x)->fill, fill)) { dec(fill); return x; }
      if (reusable(x)) {
        dec(c(FillArr, x)->fill);
        c(FillArr, x)->fill = fill;
        return x;
      }
      break;
  }
  usz ia = a(x)->ia;
  if (isNum(fill)) {
    if (v(x)->type==t_harr) {
      B* xp = harr_ptr(x);
      {
        B r = m_i32arrc(x); i32* rp = i32arr_ptr(r);
        for (usz i = 0; i < ia; i++) {
          B c = xp[i];
          if (!q_i32(c)) { dec(r); goto h_f64; }
          rp[i] = o2iu(c);
        }
        dec(x);
        return r;
      }
      h_f64: {
        B r = m_f64arrc(x); f64* rp = f64arr_ptr(r);
        for (usz i = 0; i < ia; i++) {
          B c = xp[i];
          if (!q_f64(c)) { dec(r); goto base; }
          rp[i] = o2fu(c);
        }
        dec(x);
        return r;
      }
    } else {
      BS2B xgetU = TI(x).getU;
      {
        B r = m_i32arrc(x); i32* rp = i32arr_ptr(r);
        for (usz i = 0; i < ia; i++) {
          B c = xgetU(x, i);
          if (!q_i32(c)) { dec(r); goto g_f64; }
          rp[i] = o2iu(c);
        }
        dec(x);
        return r;
      }
      g_f64: {
        B r = m_f64arrc(x); f64* rp = f64arr_ptr(r);
        for (usz i = 0; i < ia; i++) {
          B c = xgetU(x, i);
          if (!q_f64(c)) { dec(r); goto base; }
          rp[i] = o2fu(c);
        }
        dec(x);
        return r;
      }
      
      // bool ints = true;
      // for (usz i = 0; i < ia; i++) {
      //   B c = xgetU(x, i);
      //   if (!isNum(c)) goto base;
      //   if (!q_i32(c)) ints = false;
      // }
      // if (ints) {
      //   B r = m_i32arrc(x); i32* rp = i32arr_ptr(r);
      //   for (usz i = 0; i < ia; i++) rp[i] = o2iu(xgetU(x, i));
      //   dec(x);
      //   return r;
      // } else {
      //   B r = m_f64arrc(x); f64* rp = f64arr_ptr(r);
      //   for (usz i = 0; i < ia; i++) rp[i] = o2fu(xgetU(x, i));
      //   dec(x);
      //   return r;
      // }
    }
  } else if (isC32(fill)) {
    B r = m_c32arrc(x); u32* rp = c32arr_ptr(r);
    BS2B xgetU = TI(x).getU;
    for (usz i = 0; i < ia; i++) {
      B c = xgetU(x, i);
      if (!isC32(c)) { dec(r); goto base; }
      rp[i] = o2c(c);
    }
    dec(x);
    return r;
  }
  base:
  B r = m_arr(fsizeof(FillArr,a,B,ia), t_fillarr);
  arr_shCopy(r, x);
  c(FillArr,r)->fill = fill;
  B* a = c(FillArr,r)->a;
  BS2B xget = TI(x).get;
  for (usz i = 0; i < ia; i++) a[i] = xget(x,i);
  dec(x);
  return r;
}
B qWithFill(B x, B fill) { // consumes both
  if (noFill(fill)) return x;
  return withFill(x, fill);
}
