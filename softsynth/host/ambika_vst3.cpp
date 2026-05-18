#include "plugin_shared.h"

#include <pluginterfaces/base/funknown.h>
#include <pluginterfaces/base/fstrdefs.h>
#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivsteditcontroller.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>
#include <pluginterfaces/vst/ivstevents.h>
#include <pluginterfaces/vst/vsttypes.h>
#include <public.sdk/source/vst/vstaudioeffect.h>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <public.sdk/source/vst/vstparameters.h>
#include <public.sdk/source/main/pluginfactory.h>
#include <public.sdk/source/common/pluginview.h>

using namespace Steinberg;
using namespace Steinberg::Vst;

static const FUID kAmbikaProcessorUID(0xE2C8A17B, 0x9A3F4D87, 0xB5F21C6E, 0x9D438A71);
static const FUID kAmbikaControllerUID(0xA5F63B28, 0xC1E9470D, 0x8D4A6E39, 0x7B52F80C);

// ---------------------------------------------------------------------------
// Processor
// ---------------------------------------------------------------------------
class AmbikaProcessor : public AudioEffect {
 public:
  AmbikaProcessor() { setControllerClass(kAmbikaControllerUID); }

  static FUnknown* Create(void*) { return (IAudioProcessor*)new AmbikaProcessor(); }

  tresult PLUGIN_API initialize(FUnknown* context) override {
    tresult r = AudioEffect::initialize(context);
    if (r != kResultOk) return r;
    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
    addEventInput(STR16("MIDI In"), 16);
    return kResultOk;
  }

  tresult PLUGIN_API terminate() override {
    return AudioEffect::terminate();
  }

  tresult PLUGIN_API setBusArrangements(SpeakerArrangement* inputs,
                                         int32 numIn,
                                         SpeakerArrangement* outputs,
                                         int32 numOut) override {
    if (numIn == 0 && numOut == 1 && outputs[0] == SpeakerArr::kStereo)
      return kResultOk;
    return kResultFalse;
  }

  tresult PLUGIN_API setProcessing(TBool state) override {
    return AudioEffect::setProcessing(state);
  }

  tresult PLUGIN_API process(ProcessData& data) override {
    if (data.inputParameterChanges) {
      int32 count = data.inputParameterChanges->getParameterCount();
      for (int32 i = 0; i < count; ++i) {
        IParamValueQueue* q = data.inputParameterChanges->getParameterData(i);
        if (q) {
          ParamValue v;
          int32 sampleOffset;
          if (q->getPoint(q->getPointCount() - 1, sampleOffset, v) == kResultOk)
            synth_.SetParam(q->getParameterId(), (float)v);
        }
      }
    }

    if (data.inputEvents) {
      int32 count = data.inputEvents->getEventCount();
      for (int32 i = 0; i < count; ++i) {
        Event ev;
        if (data.inputEvents->getEvent(i, ev) == kResultOk) {
          if (ev.type == Event::kNoteOnEvent)
            synth_.NoteOn(ev.noteOn.channel, ev.noteOn.pitch,
                          (uint8_t)(ev.noteOn.velocity * 127.0f));
          else if (ev.type == Event::kNoteOffEvent)
            synth_.NoteOff(ev.noteOff.channel, ev.noteOff.pitch,
                           (uint8_t)(ev.noteOff.velocity * 127.0f));
        }
      }
    }

    if (data.outputs && data.numOutputs > 0) {
      float* outL = data.outputs[0].channelBuffers32[0];
      float* outR = data.outputs[0].channelBuffers32[1];
      if (!outL || !outR) return kResultOk;
      float* outputs[2] = { outL, outR };
      synth_.ProcessBlock(outputs, 2, data.numSamples);
    }

    return kResultOk;
  }

  tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) override {
    return symbolicSampleSize == kSample32 ? kResultTrue : kResultFalse;
  }

 private:
  PluginVoiceManager synth_;
};

// ---------------------------------------------------------------------------
// Controller — EditControllerEx1 adds IUnitInfo for parameter grouping
// ---------------------------------------------------------------------------
static int32 kUnitRootId = 0;
static int32 kUnitOscId = 1;
static int32 kUnitFilterId = 2;
static int32 kUnitMixId = 3;
static int32 kUnitEnvId = 4;
static int32 kUnitLFOId = 5;
static int32 kUnitGlobalId = 6;
static int32 kUnitModId = 7;
static int32 kUnitControlId = 8;

class AmbikaController : public EditControllerEx1 {
 public:
  AmbikaController() : EditControllerEx1() {}

  static FUnknown* Create(void*) {
    return static_cast<IEditController*>(new AmbikaController());
  }

  tresult PLUGIN_API initialize(FUnknown* context) override {
    tresult r = EditControllerEx1::initialize(context);
    if (r != kResultOk) return r;

    for (int i = 0; i < PARAM_COUNT; ++i) {
      String128 name;
      str8ToStr16(name, PluginVoiceManager::GetName(i));
      int32 unitId = kUnitRootId;
      if (i <= 7)                unitId = kUnitOscId;
      else if (i <= 11)          unitId = kUnitFilterId;
      else if (i <= 16)          unitId = kUnitMixId;
      else if (i <= 28)          unitId = kUnitEnvId;
      else if (i <= 30)          unitId = kUnitLFOId;
      else if (i <= 32)          unitId = kUnitGlobalId;
      else if (i <= 74)          unitId = kUnitModId;
      else                       unitId = kUnitControlId;

      auto p = new RangeParameter(
          name, i, nullptr,
          0.0, 1.0,
          PluginVoiceManager::GetDefault(i),
          0,
          ParameterInfo::kCanAutomate,
          unitId);
      parameters.addParameter(p);
    }

    return kResultOk;
  }

  tresult PLUGIN_API setParamNormalized(ParamID tag, ParamValue value) override {
    return EditControllerEx1::setParamNormalized(tag, value);
  }

  IPlugView* PLUGIN_API createView(FIDString name) override {
    return nullptr;
  }

  // --- IUnitInfo ---
  int32 PLUGIN_API getUnitCount() override {
    return 9;
  }

  tresult PLUGIN_API getUnitInfo(int32 unitIndex, UnitInfo& info) override {
    static const char* names[] = {
      "Root", "Oscillators", "Filter", "Mix", "Envelopes",
      "LFO", "Global", "Mod Matrix", "Controls"
    };
    if (unitIndex < 0 || unitIndex >= 9) return kResultFalse;
    info.id = unitIndex;
    info.parentUnitId = unitIndex == 0 ? kNoParentUnitId : kUnitRootId;
    str8ToStr16(info.name, names[unitIndex]);
    info.programListId = kNoProgramListId;
    return kResultOk;
  }

  tresult PLUGIN_API getUnitByBus(MediaType type, BusDirection dir,
                                   int32 busIndex, int32 channel,
                                   int32& unitId) override {
    if (type == kAudio && dir == kOutput && busIndex == 0) {
      unitId = kUnitRootId;
      return kResultOk;
    }
    return kResultFalse;
  }

  int32 PLUGIN_API getProgramListCount() override { return 1; }

  tresult PLUGIN_API getProgramListInfo(int32 listIndex,
                                         ProgramListInfo& info) override {
    if (listIndex != 0) return kResultFalse;
    info.id = 1;
    info.programCount = kNumPatches;
    str8ToStr16(info.name, "Presets");
    return kResultOk;
  }

  tresult PLUGIN_API getProgramName(ProgramListID listId, int32 programIndex,
                                     String128 name) override {
    if (listId != 1 || programIndex < 0 || programIndex >= kNumPatches)
      return kResultFalse;
    str8ToStr16(name, kPatchNames[programIndex]);
    return kResultOk;
  }


};

// ---------------------------------------------------------------------------
// Factory entry
// ---------------------------------------------------------------------------
BEGIN_FACTORY("https://ambika.opencode/softsynth",
              "https://ambika.opencode/softsynth",
              "contact@example.com",
              PFactoryInfo::kNoFlags)

  DEF_VST3_CLASS("Ambika",
                 "Instrument|Synth",
                 Vst::kDistributable,
                 "1.0.0",
                 INLINE_UID_FROM_FUID(kAmbikaProcessorUID),
                 AmbikaProcessor::Create,
                 INLINE_UID_FROM_FUID(kAmbikaControllerUID),
                 AmbikaController::Create)

END_FACTORY
