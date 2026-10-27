// Renders the docs' listening page: Death's Hand's six documented patches and
// showcase clips (test/patches.hpp), and Doombringer's (test/doom_patches.hpp),
// each as a 48 kHz 16-bit mono WAV normalised to -1 dBFS.
//
//   build/render <dir>     (make -C test renders)
//
// Parameters update once per 48 samples, like the firmware's 1 kHz control rate.
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "doom_patches.hpp"
#include "patches.hpp"

using namespace cosc;

static constexpr float kFs = 48000.f;

static bool writeWav(const std::string& path, const std::vector<float>& x) {
    float peak = 1e-9f;
    for (float v : x) peak = std::fmax(peak, std::fabs(v));
    const float g = 0.891f / peak; // -1 dBFS
    FILE*       f = std::fopen(path.c_str(), "wb");
    if (!f) {
        std::perror(path.c_str());
        return false;
    }
    const uint32_t n = static_cast<uint32_t>(x.size()), bytes = n * 2, rate = 48000;
    auto           u32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
    auto           u16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
    std::fwrite("RIFF", 1, 4, f);
    u32(36 + bytes);
    std::fwrite("WAVEfmt ", 1, 8, f);
    u32(16);
    u16(1); // PCM
    u16(1); // mono
    u32(rate);
    u32(rate * 2);
    u16(2);
    u16(16);
    std::fwrite("data", 1, 4, f);
    u32(bytes);
    for (float v : x) {
        const int16_t s = static_cast<int16_t>(std::lround(std::fmax(-1.f, std::fmin(1.f, v * g)) * 32767.f));
        std::fwrite(&s, 2, 1, f);
    }
    std::fclose(f);
    std::printf("  wrote %s\n", path.c_str());
    return true;
}

// automate(t seconds, panel, vca) at the control rate; short fades at the ends.
template <typename Automate>
static std::vector<float> render(float seconds, Automate automate) {
    static ComplexOsc<4> osc;
    osc.init(kFs);
    PanelState s;
    float      vca = 1.f;
    automate(0.f, s, vca);
    osc.setParams(panelToParams(s));
    osc.snapSmoothers();
    const int          n = static_cast<int>(seconds * kFs);
    std::vector<float> out(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / kFs;
        if (i % 48 == 0) {
            s   = PanelState{};
            vca = 1.f;
            automate(t, s, vca);
            osc.setParams(panelToParams(s));
        }
        float p, m;
        osc.process(0.f, p, m);
        const float fade            = std::fmin(1.f, std::fmin(t / 0.02f, (seconds - t) / 0.3f));
        out[static_cast<size_t>(i)] = p * vca * fade;
    }
    return out;
}

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "out/listen";
    bool              ok  = true;
    std::printf("patches\n");
    for (const Patch& p : patches()) {
        const float seconds = 10.f;
        ok                  = writeWav(dir + "/" + p.file, render(seconds, [&](float t, PanelState& s, float&) { p.automate(t / seconds, s); })) && ok;
    }
    std::printf("showcase\n");
    for (const Clip& c : showcase()) ok = writeWav(dir + "/" + c.file + ".wav", render(c.seconds, c.automate)) && ok;
    std::printf("doombringer\n");
    for (const doom::DoomClip& c : doom::doomClips()) ok = writeWav(dir + "/doom_" + c.file + ".wav", doom::renderClip(c, kFs)) && ok;
    return ok ? 0 : 1;
}
