// VCV Rack adapter. The engine is the exact same ../dsp code the patch.Init
// firmware runs, and the panel mirrors the hardware: pitch and the three
// character macros on top, the patch.Init's shift-layer settings below. Knobs
// and CVs go through the same cosc::panelToParams the firmware uses.
#include "plugin.hpp"
#include "PanelLayout.hpp"
#include "dsp/Panel.hpp"

struct ComplexOscModule : Module {
    enum ParamId {
        PITCH_PARAM,
        FLESH_PARAM,
        MALICE_PARAM,
        LEGION_PARAM,
        RATIO_PARAM,
        MOD_SHAPE_PARAM,
        SYMMETRY_PARAM,
        FINE_PARAM,
        RATIO_LOCK_PARAM,
        ROUTING_PARAM,
        PARAMS_LEN
    };
    enum InputId { VOCT_INPUT, FLESH_INPUT, MALICE_INPUT, LEGION_INPUT, SYMMETRY_INPUT, EXT_FM_INPUT, SYNC_INPUT, ROUTING_INPUT, INPUTS_LEN };
    enum OutputId { PRIMARY_OUTPUT, MOD_OUTPUT, OUTPUTS_LEN };
    enum LightId { LIGHTS_LEN };

    cosc::ComplexOsc<4> osc;
    dsp::SchmittTrigger syncTrigger, routingTrigger;
    dsp::ClockDivider   paramDivider;

    // Shows the quantized ratio (lock mode) or modulator frequency (free mode).
    struct RatioQuantity : ParamQuantity {
        std::string getDisplayValueString() override {
            auto* m = dynamic_cast<ComplexOscModule*>(module);
            float v = getValue();
            if (!m || m->params[RATIO_LOCK_PARAM].getValue() > 0.5f) return string::f("%g : 1", cosc::quantizeRatio(v));
            return string::f("%.3f Hz", cosc::voltsToHz(v * 11.f - 8.f));
        }
    };

    ComplexOscModule() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        const cosc::PanelState d;
        // Pitch spans 6 octaves around C4: Hz = (C4 / 8) * 64^knob.
        configParam(PITCH_PARAM, 0.f, 1.f, d.pitch, "Pitch", " Hz", 64.f, dsp::FREQ_C4 / 8.f);
        configParam(FLESH_PARAM, 0.f, 1.f, 0.f, "Flesh: FM, fold, asymmetry", "%", 0.f, 100.f);
        configParam(MALICE_PARAM, 0.f, 1.f, 0.f, "Malice: feedback, chaos", "%", 0.f, 100.f);
        configParam(LEGION_PARAM, 0.f, 1.f, 0.f, "Legion: sub, voices, drift", "%", 0.f, 100.f);
        configParam<RatioQuantity>(RATIO_PARAM, 0.f, 1.f, d.ratio, "Ratio / modulator pitch");
        configSwitch(MOD_SHAPE_PARAM, 0.f, 4.f, 0.f, "Modulator shape", {"Sine", "Triangle", "Saw", "Square", "Lorenz"});
        configParam(SYMMETRY_PARAM, 0.f, 1.f, d.symmetry, "Symmetry trim", "%", 0.f, 200.f, -100.f);
        configParam(FINE_PARAM, 0.f, 1.f, d.fine, "Fine tune", " semitones", 0.f, 2.f, -1.f);
        configSwitch(RATIO_LOCK_PARAM, 0.f, 1.f, 1.f, "Modulator", {"Free", "Ratio lock"});
        configSwitch(ROUTING_PARAM, 0.f, 3.f, 0.f, "Routing", {"FM", "FM + timbre mod", "FM + AM", "FM + ring"});
        configInput(VOCT_INPUT, "Pitch 1V/oct");
        configInput(FLESH_INPUT, "Flesh CV (+-5 V sweeps the range)");
        configInput(MALICE_INPUT, "Malice CV (+-5 V sweeps the range)");
        configInput(LEGION_INPUT, "Legion CV (+-5 V sweeps the range)");
        configInput(SYMMETRY_INPUT, "Symmetry CV");
        configInput(EXT_FM_INPUT, "External FM (audio)");
        configInput(SYNC_INPUT, "Hard sync");
        configInput(ROUTING_INPUT, "Next routing (trigger)");
        configOutput(PRIMARY_OUTPUT, "Primary");
        configOutput(MOD_OUTPUT, "Modulator");
        paramDivider.setDivision(4);
        osc.init(48000.f);
    }

    void updateParams() {
        cosc::PanelState s;
        s.pitch  = params[PITCH_PARAM].getValue();
        s.flesh  = params[FLESH_PARAM].getValue();
        s.malice = params[MALICE_PARAM].getValue();
        s.legion = params[LEGION_PARAM].getValue();
        // The snap knobs return exact integers; map them to the centre of the patch.Init zones.
        s.ratio      = params[RATIO_PARAM].getValue();
        s.shape      = (params[MOD_SHAPE_PARAM].getValue() + 0.5f) / static_cast<float>(cosc::ModShape::Count);
        s.symmetry   = params[SYMMETRY_PARAM].getValue();
        s.fine       = params[FINE_PARAM].getValue();
        s.ratioLock  = params[RATIO_LOCK_PARAM].getValue() > 0.5f;
        s.routing    = static_cast<int>(params[ROUTING_PARAM].getValue());
        s.pitchCv    = inputs[VOCT_INPUT].getVoltage();
        s.fleshCv    = inputs[FLESH_INPUT].getVoltage();
        s.maliceCv   = inputs[MALICE_INPUT].getVoltage();
        s.legionCv   = inputs[LEGION_INPUT].getVoltage();
        s.symmetryCv = inputs[SYMMETRY_INPUT].getVoltage();
        osc.setParams(cosc::panelToParams(s));
    }

    void process(const ProcessArgs& args) override {
        if (args.sampleRate != osc.sampleRate()) {
            osc.init(args.sampleRate);
            updateParams();
            osc.snapSmoothers();
        }
        if (syncTrigger.process(inputs[SYNC_INPUT].getVoltage(), 0.1f, 1.f)) osc.sync();
        if (routingTrigger.process(inputs[ROUTING_INPUT].getVoltage(), 0.1f, 1.f))
            params[ROUTING_PARAM].setValue(static_cast<float>((static_cast<int>(params[ROUTING_PARAM].getValue()) + 1) % cosc::kNumRoutings));
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
        addParam(createParamCentered<RoundLargeBlackKnob>(at(panel::FLESH_PARAM), module, M::FLESH_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(at(panel::MALICE_PARAM), module, M::MALICE_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(at(panel::LEGION_PARAM), module, M::LEGION_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(at(panel::RATIO_PARAM), module, M::RATIO_PARAM));
        addParam(createParamCentered<RoundBlackSnapKnob>(at(panel::MOD_SHAPE_PARAM), module, M::MOD_SHAPE_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(at(panel::SYMMETRY_PARAM), module, M::SYMMETRY_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(at(panel::FINE_PARAM), module, M::FINE_PARAM));
        addParam(createParamCentered<CKSS>(at(panel::RATIO_LOCK_PARAM), module, M::RATIO_LOCK_PARAM));
        addParam(createParamCentered<RoundBlackSnapKnob>(at(panel::ROUTING_PARAM), module, M::ROUTING_PARAM));

        addInput(createInputCentered<PJ301MPort>(at(panel::VOCT_INPUT), module, M::VOCT_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::FLESH_INPUT), module, M::FLESH_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::MALICE_INPUT), module, M::MALICE_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::LEGION_INPUT), module, M::LEGION_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::SYMMETRY_INPUT), module, M::SYMMETRY_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::EXT_FM_INPUT), module, M::EXT_FM_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::SYNC_INPUT), module, M::SYNC_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::ROUTING_INPUT), module, M::ROUTING_INPUT));

        addOutput(createOutputCentered<PJ301MPort>(at(panel::MOD_OUTPUT), module, M::MOD_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(at(panel::PRIMARY_OUTPUT), module, M::PRIMARY_OUTPUT));
    }
};

Model* modelComplexOsc = createModel<ComplexOscModule, ComplexOscWidget>("DeathsHand");
