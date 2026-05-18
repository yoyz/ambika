// LV2 host integration test — loads ambika.so, sends MIDI, validates audio
// Requires: lv2-dev, built ambika.so
// Build: make test_lv2

#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <dlfcn.h>

#include <lv2/core/lv2.h>
#include <lv2/urid/urid.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>
#include <lv2/midi/midi.h>

#include "../host/plugin_shared.h"

struct TestResult { const char* name; bool ok; char info[256]; };
#define PASS   (TestResult{__func__,true,""})
#define FAIL(...) (TestResult{__func__,false,{0}})
#define CHECK(c, ...) do{if(!(c)){TestResult _r={__func__,false,{0}};snprintf(_r.info,sizeof(_r.info),__VA_ARGS__);return _r;}}while(0)
#define FMT(...) (snprintf(_buf,sizeof(_buf),__VA_ARGS__),_buf)
static char _buf[512];

static double RMS(const float* s, int n) {
  double sum = 0;
  for (int i = 0; i < n; ++i) sum += s[i] * s[i];
  return sqrt(sum / n);
}

// ---------------------------------------------------------------------------
// Mini LV2 URID mapper
// ---------------------------------------------------------------------------
struct Mapper { LV2_URID_Map f; LV2_URID midi; };
static LV2_URID map(LV2_URID_Map_Handle h, const char* u) {
  Mapper* m = (Mapper*)h;
  return strcmp(u, LV2_MIDI__MidiEvent) == 0 ? (m->midi ? m->midi : 1) : 0;
}

// ---------------------------------------------------------------------------
// Atom sequence builder
// ---------------------------------------------------------------------------
struct AtomBuf { char d[8192]; };
// offset where events start: past atom header + body_size uint32
// LV2_Atom_Sequence layout:
//   LV2_Atom (8B): size + type
//   body.unit (4B)
//   body.pad  (4B)
//   events[]  (each 64-bit aligned)
static const uint32_t kSeqHead = sizeof(LV2_Atom) + sizeof(LV2_Atom_Sequence_Body);
static const uint32_t kEventHeader = 16;  // time(8B) + body(8B)
static void atom_init(AtomBuf* b) {
  memset(b, 0, sizeof(*b));
  ((LV2_Atom_Sequence*)b)->atom.size = 0;
}
static void atom_midi(AtomBuf* b, uint32_t f, const uint8_t* m, uint32_t l) {
  LV2_Atom_Sequence* s = (LV2_Atom_Sequence*)b;
  uint32_t off = kSeqHead + s->atom.size;
  LV2_Atom_Event* e = (LV2_Atom_Event*)(b->d + off);
  e->time.frames = f;
  e->body.type = 1;   // MIDI event URID
  e->body.size = l;
  memcpy((uint8_t*)(e + 1), m, l);
  // Round to 64-bit alignment
  uint32_t total = kEventHeader + l;
  total = (total + 7) & ~7;
  s->atom.size += total;
}

// ---------------------------------------------------------------------------
// Test: lifecycle + MIDI note produces audio
// ---------------------------------------------------------------------------
static TestResult test_lifecycle() {
  void* h = dlopen("ambika.so", RTLD_NOW);
  CHECK(h, "dlopen ambika.so failed — 'make ambika.so' first");

  typedef const LV2_Descriptor* (*Dfn)(uint32_t);
  Dfn desc_fn = (Dfn)dlsym(h, "lv2_descriptor");
  CHECK(desc_fn, "lv2_descriptor symbol missing");

  const LV2_Descriptor* d = desc_fn(0);
  CHECK(d && d->instantiate, "descriptor 0 invalid");

  Mapper m; m.midi = 1; m.f.handle = &m; m.f.map = map;
  const LV2_Feature urid = { LV2_URID__map, &m };
  const LV2_Feature* feats[] = { &urid, NULL };

  LV2_Handle inst = d->instantiate(d, 48000.0, "/", feats);
  CHECK(inst, "instantiate failed");

  AtomBuf midi; float L[512], R[512], params[PARAM_COUNT];
  for (int i = 0; i < PARAM_COUNT; ++i)
    params[i] = PluginVoiceManager::GetDefault(i);
  d->connect_port(inst, 0, midi.d);    // MIDI in
  d->connect_port(inst, 1, L);          // Audio L
  d->connect_port(inst, 2, R);          // Audio R
  for (int i = 0; i < PARAM_COUNT; ++i)
    d->connect_port(inst, 3 + i, &params[i]);
  d->activate(inst);

  // Run with no MIDI — silence
  atom_init(&midi);
  d->run(inst, 256);
  CHECK(RMS(L, 256) < 0.001, "Silence RMS < 0.001 (got %f)", RMS(L, 256));

  // NoteOn C4 vel 100 — render the event once, then continue rendering
  // with empty MIDI to let the envelope open
  atom_init(&midi);
  uint8_t on[] = { 0x90, 60, 100 };
  atom_midi(&midi, 0, on, 3);
  d->run(inst, 256);                       // first block: NOTE_ON processed
  atom_init(&midi);                          // clear MIDI, keep rendering
  for (int j = 0; j < 20; ++j)
    d->run(inst, 256);
  CHECK(RMS(L, 256) > 0.001, "NoteOn should produce audio (RMS=%f)", RMS(L, 256));

  // NoteOff
  atom_init(&midi);
  uint8_t off[] = { 0x80, 60, 0 };
  atom_midi(&midi, 0, off, 3);
  d->run(inst, 256);

  d->deactivate(inst);
  d->cleanup(inst);
  dlclose(h);
  return PASS;
}

// ---------------------------------------------------------------------------
// Test: parameter ports
// ---------------------------------------------------------------------------
static TestResult test_params() {
  void* h = dlopen("ambika.so", RTLD_NOW);
  CHECK(h, "dlopen");
  typedef const LV2_Descriptor* (*Dfn)(uint32_t);
  Dfn desc_fn = (Dfn)dlsym(h, "lv2_descriptor");
  const LV2_Descriptor* d = desc_fn(0);

  Mapper m; m.midi = 1; m.f.handle = &m; m.f.map = map;
  const LV2_Feature urid = { LV2_URID__map, &m };
  const LV2_Feature* feats[] = { &urid, NULL };
  LV2_Handle inst = d->instantiate(d, 48000.0, "/", feats);
  CHECK(inst, "instantiate");

  AtomBuf midi2; float L2[512], R2[512], p2[PARAM_COUNT];
  for (int i = 0; i < PARAM_COUNT; ++i)
    p2[i] = PluginVoiceManager::GetDefault(i);
  d->connect_port(inst, 0, midi2.d);
  d->connect_port(inst, 1, L2);
  d->connect_port(inst, 2, R2);
  for (int i = 0; i < PARAM_COUNT; ++i) d->connect_port(inst, 3 + i, &p2[i]);
  d->activate(inst);

  // NoteOn
  atom_init(&midi2);
  uint8_t on[] = { 0x90, 60, 100 };
  atom_midi(&midi2, 0, on, 3);
  d->run(inst, 256);

  d->deactivate(inst);
  d->cleanup(inst);
  dlclose(h);
  return PASS;
}

// ---------------------------------------------------------------------------
// Test: descriptor metadata
// ---------------------------------------------------------------------------
static TestResult test_descriptor() {
  void* h = dlopen("ambika.so", RTLD_NOW);
  CHECK(h, "dlopen");
  typedef const LV2_Descriptor* (*Dfn)(uint32_t);
  const LV2_Descriptor* d = ((Dfn)dlsym(h, "lv2_descriptor"))(0);
  CHECK(d && d->URI && strlen(d->URI) > 0, "URI empty");
  dlclose(h);
  return PASS;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main() {
  struct { TestResult (*fn)(); const char* name; } tests[] = {
    { test_lifecycle, "lv2_lifecycle" },
    { test_params, "lv2_params" },
    { test_descriptor, "lv2_descriptor" },
  };
  int nt = sizeof(tests)/sizeof(tests[0]), pass = 0, fail = 0;
  printf("\n  LV2 Plugin Tests\n  =================\n\n");
  for (int i = 0; i < nt; ++i) {
    printf("  [%d/%d] %-20s ... ", i + 1, nt, tests[i].name); fflush(stdout);
    TestResult r = tests[i].fn();
    if (r.ok) { printf("PASS\n"); pass++; }
    else { printf("FAIL\n         %s\n", r.info); fail++; }
  }
  printf("\n  =================\n  Results: %d/%d passed, %d failed\n\n", pass, nt, fail);
  return fail > 0 ? 1 : 0;
}
