// VCV Rack adapter. The engine is the exact same ../dsp code the patch.Init
// firmware runs — this file only maps Rack params/ports to cosc::Params.
#include "plugin.hpp"
#include "PanelLayout.hpp"
#include "ComplexOsc.hpp"

struct ComplexOscModule : Module {
    enum ParamId {
        PITCH_PARAM,
        FINE_PARAM,
        MOD_PITCH_PARAM,
        FM_INDEX_PARAM,
        TIMBRE_PARAM,
        SYMMETRY_PARAM,
        AM_PARAM,
        TIMBRE_MOD_PARAM,
        RATIO_LOCK_PARAM,
        MOD_SHAPE_PARAM,
        FM_CV_ATTEN_PARAM,
        TIMBRE_CV_ATTEN_PARAM,
        FEEDBACK_PARAM,
        CHAOS_PARAM,
        FEEDBACK_CV_ATTEN_PARAM,
        CHAOS_CV_ATTEN_PARAM,
        // Unison stack. Not on the panel (16 HP is full); set from the context menu.
        VOICES_PARAM,
        SPREAD_PARAM,
        DRIFT_PARAM,
        SUB_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        VOCT_INPUT,
        MOD_VOCT_INPUT,
        FM_INDEX_INPUT,
        TIMBRE_INPUT,
        SYMMETRY_INPUT,
        AM_INPUT,
        EXT_FM_INPUT,
        SYNC_INPUT,
        FEEDBACK_INPUT,
        CHAOS_INPUT,
        INPUTS_LEN
    };
    enum OutputId { PRIMARY_OUTPUT, MOD_OUTPUT, OUTPUTS_LEN };
    enum LightId { LIGHTS_LEN };

    cosc::ComplexOsc<4> osc;
    dsp::SchmittTrigger syncTrigger;
    dsp::ClockDivider   paramDivider;

    // Shows the quantized ratio (lock mode) or modulator frequency (free mode).
    struct ModPitchQuantity : ParamQuantity {
        std::string getDisplayValueString() override {
            auto* m = dynamic_cast<ComplexOscModule*>(module);
            float v = getValue();
            if (!m || m->params[RATIO_LOCK_PARAM].getValue() > 0.5f) return string::f("%g : 1", cosc::quantizeRatio(v));
            return string::f("%.3f Hz", cosc::voltsToHz(v * 11.f - 8.f));
        }
    };

    ComplexOscModule() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(PITCH_PARAM, -3.f, 3.f, 0.f, "Pitch", " Hz", 2.f, dsp::FREQ_C4);
        configParam(FINE_PARAM, -1.f, 1.f, 0.f, "Fine", " semitones");
        configParam<ModPitchQuantity>(MOD_PITCH_PARAM, 0.f, 1.f, 0.25f, "Ratio / modulator pitch");
        configParam(FM_INDEX_PARAM, 0.f, 1.f, 0.f, "FM index", "%", 0.f, 100.f);
        configParam(TIMBRE_PARAM, 0.f, 1.f, 0.f, "Timbre (fold)", "%", 0.f, 100.f);
        configParam(SYMMETRY_PARAM, -1.f, 1.f, 0.f, "Symmetry", "%", 0.f, 100.f);
        configParam(AM_PARAM, 0.f, 1.f, 0.f, "AM → ring mod", "%", 0.f, 100.f);
        configParam(TIMBRE_MOD_PARAM, 0.f, 1.f, 0.f, "Modulator → timbre", "%", 0.f, 100.f);
        configSwitch(RATIO_LOCK_PARAM, 0.f, 1.f, 1.f, "Modulator", {"Free", "Ratio lock"});
        configSwitch(MOD_SHAPE_PARAM, 0.f, 4.f, 0.f, "Modulator shape", {"Sine", "Triangle", "Saw", "Square", "Lorenz"});
        configParam(FM_CV_ATTEN_PARAM, -1.f, 1.f, 0.f, "FM index CV", "%", 0.f, 100.f);
        configParam(TIMBRE_CV_ATTEN_PARAM, -1.f, 1.f, 0.f, "Timbre CV", "%", 0.f, 100.f);
        configParam(FEEDBACK_PARAM, 0.f, 1.f, 0.f, "Self-feedback", "%", 0.f, 100.f);
        configParam(CHAOS_PARAM, 0.f, 1.f, 0.f, "Chaos (primary → modulator FM)", "%", 0.f, 100.f);
        configParam(FEEDBACK_CV_ATTEN_PARAM, -1.f, 1.f, 0.f, "Feedback CV", "%", 0.f, 100.f);
        configParam(CHAOS_CV_ATTEN_PARAM, -1.f, 1.f, 0.f, "Chaos CV", "%", 0.f, 100.f);
        configSwitch(VOICES_PARAM, 1.f, 4.f, 1.f, "Unison voices", {"1", "2", "3", "4"});
        configParam(SPREAD_PARAM, 0.f, 1.f, 0.f, "Unison spread", "%", 0.f, 100.f);
        configParam(DRIFT_PARAM, 0.f, 1.f, 0.f, "Unison drift", "%", 0.f, 100.f);
        configParam(SUB_PARAM, 0.f, 1.f, 0.f, "Sub (one octave down)", "%", 0.f, 100.f);
        configInput(VOCT_INPUT, "Primary 1V/oct");
        configInput(MOD_VOCT_INPUT, "Modulator 1V/oct (free) / ratio offset (lock)");
        configInput(FM_INDEX_INPUT, "FM index CV");
        configInput(TIMBRE_INPUT, "Timbre CV");
        configInput(SYMMETRY_INPUT, "Symmetry CV");
        configInput(AM_INPUT, "AM CV (0–10 V)");
        configInput(EXT_FM_INPUT, "External FM (audio)");
        configInput(SYNC_INPUT, "Hard sync");
        configInput(FEEDBACK_INPUT, "Feedback CV");
        configInput(CHAOS_INPUT, "Chaos CV");
        configOutput(PRIMARY_OUTPUT, "Primary");
        configOutput(MOD_OUTPUT, "Modulator");
        paramDivider.setDivision(4);
        osc.init(48000.f);
    }

    void updateParams() {
        cosc::Params p;
        p.pitchV = params[PITCH_PARAM].getValue() + params[FINE_PARAM].getValue() / 12.f + inputs[VOCT_INPUT].getVoltage();

        const float modKnob = params[MOD_PITCH_PARAM].getValue();
        p.ratioLock         = params[RATIO_LOCK_PARAM].getValue() > 0.5f;
        if (p.ratioLock) p.ratio = cosc::quantizeRatio(modKnob + inputs[MOD_VOCT_INPUT].getVoltage() * 0.1f);
        else p.modPitchV = modKnob * 11.f - 8.f + inputs[MOD_VOCT_INPUT].getVoltage();

        p.fmIndex   = params[FM_INDEX_PARAM].getValue() + params[FM_CV_ATTEN_PARAM].getValue() * inputs[FM_INDEX_INPUT].getVoltage() / 5.f;
        p.timbre    = params[TIMBRE_PARAM].getValue() + params[TIMBRE_CV_ATTEN_PARAM].getValue() * inputs[TIMBRE_INPUT].getVoltage() / 5.f;
        p.symmetry  = params[SYMMETRY_PARAM].getValue() + inputs[SYMMETRY_INPUT].getVoltage() / 5.f;
        p.amDepth   = params[AM_PARAM].getValue() + inputs[AM_INPUT].getVoltage() / 10.f;
        p.timbreMod = params[TIMBRE_MOD_PARAM].getValue();
        p.feedback  = params[FEEDBACK_PARAM].getValue() + params[FEEDBACK_CV_ATTEN_PARAM].getValue() * inputs[FEEDBACK_INPUT].getVoltage() / 5.f;
        p.chaos     = params[CHAOS_PARAM].getValue() + params[CHAOS_CV_ATTEN_PARAM].getValue() * inputs[CHAOS_INPUT].getVoltage() / 5.f;
        p.modShape  = static_cast<cosc::ModShape>(static_cast<int>(params[MOD_SHAPE_PARAM].getValue()));
        p.voices    = static_cast<int>(params[VOICES_PARAM].getValue());
        p.spread    = params[SPREAD_PARAM].getValue();
        p.drift     = params[DRIFT_PARAM].getValue();
        p.sub       = params[SUB_PARAM].getValue();
        osc.setParams(p);
    }

    void process(const ProcessArgs& args) override {
        if (args.sampleRate != osc.sampleRate()) {
            osc.init(args.sampleRate);
            updateParams();
            osc.snapSmoothers();
        }
        if (syncTrigger.process(inputs[SYNC_INPUT].getVoltage(), 0.1f, 1.f)) osc.sync();
        if (paramDivider.process()) updateParams();

        float primary, mod;
        osc.process(inputs[EXT_FM_INPUT].getVoltage(), primary, mod);
        outputs[PRIMARY_OUTPUT].setVoltage(primary);
        outputs[MOD_OUTPUT].setVoltage(mod);
    }
};

struct ComplexOscWidget : ModuleWidget {
    explicit ComplexOscWidget(ComplexOscModule* module) {
        using M = ComplexOscModule;
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ComplexOsc.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        auto at = [](panel::Pos p) { return mm2px(Vec(p.x, p.y)); };

        addParam(createParamCentered<RoundLargeBlackKnob>(at(panel::PITCH_PARAM), module, M::PITCH_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(at(panel::FINE_PARAM), module, M::FINE_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(at(panel::MOD_PITCH_PARAM), module, M::MOD_PITCH_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(at(panel::FM_INDEX_PARAM), module, M::FM_INDEX_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(at(panel::TIMBRE_PARAM), module, M::TIMBRE_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(at(panel::SYMMETRY_PARAM), module, M::SYMMETRY_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(at(panel::AM_PARAM), module, M::AM_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(at(panel::TIMBRE_MOD_PARAM), module, M::TIMBRE_MOD_PARAM));
        addParam(createParamCentered<CKSS>(at(panel::RATIO_LOCK_PARAM), module, M::RATIO_LOCK_PARAM));
        addParam(createParamCentered<RoundBlackSnapKnob>(at(panel::MOD_SHAPE_PARAM), module, M::MOD_SHAPE_PARAM));
        addParam(createParamCentered<Trimpot>(at(panel::FM_CV_ATTEN_PARAM), module, M::FM_CV_ATTEN_PARAM));
        addParam(createParamCentered<Trimpot>(at(panel::TIMBRE_CV_ATTEN_PARAM), module, M::TIMBRE_CV_ATTEN_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(at(panel::FEEDBACK_PARAM), module, M::FEEDBACK_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(at(panel::CHAOS_PARAM), module, M::CHAOS_PARAM));
        addParam(createParamCentered<Trimpot>(at(panel::FEEDBACK_CV_ATTEN_PARAM), module, M::FEEDBACK_CV_ATTEN_PARAM));
        addParam(createParamCentered<Trimpot>(at(panel::CHAOS_CV_ATTEN_PARAM), module, M::CHAOS_CV_ATTEN_PARAM));

        addInput(createInputCentered<PJ301MPort>(at(panel::VOCT_INPUT), module, M::VOCT_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::MOD_VOCT_INPUT), module, M::MOD_VOCT_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::FM_INDEX_INPUT), module, M::FM_INDEX_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::TIMBRE_INPUT), module, M::TIMBRE_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::SYMMETRY_INPUT), module, M::SYMMETRY_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::AM_INPUT), module, M::AM_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::EXT_FM_INPUT), module, M::EXT_FM_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::SYNC_INPUT), module, M::SYNC_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::FEEDBACK_INPUT), module, M::FEEDBACK_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::CHAOS_INPUT), module, M::CHAOS_INPUT));

        addOutput(createOutputCentered<PJ301MPort>(at(panel::MOD_OUTPUT), module, M::MOD_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(at(panel::PRIMARY_OUTPUT), module, M::PRIMARY_OUTPUT));
    }

    // The unison stack lives in the context menu, as on the patch.Init's third layer.
    void appendContextMenu(Menu* menu) override {
        auto* m = dynamic_cast<ComplexOscModule*>(module);
        if (!m) return;
        using M = ComplexOscModule;
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Unison"));
        menu->addChild(createIndexSubmenuItem(
            "Voices", {"1", "2", "3", "4"}, [=]() { return static_cast<size_t>(m->params[M::VOICES_PARAM].getValue()) - 1; },
            [=](size_t i) { m->params[M::VOICES_PARAM].setValue(static_cast<float>(i + 1)); }));
        for (int id : {M::SPREAD_PARAM, M::DRIFT_PARAM, M::SUB_PARAM}) {
            auto* slider       = new ui::Slider;
            slider->quantity   = m->paramQuantities[id];
            slider->box.size.x = 200.f;
            menu->addChild(slider);
        }
    }
};

Model* modelComplexOsc = createModel<ComplexOscModule, ComplexOscWidget>("DeathsHand");
