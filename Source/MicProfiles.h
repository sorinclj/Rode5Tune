#pragma once
// Rode5tune - microphone voicing profiles (pure C++, no JUCE dependency)
// NOTE: curves are *voicings* derived from the widely documented tonal character of each
// mic, relative to a flat reference. They are not measurement-matched clones.

namespace r5t
{
enum class BandType { None, LowShelf, Peak, HighShelf, HighPass };

struct BandSpec { BandType type; float freq; float gainDb; float q; };

enum PreampType { PreampNative = 0, PreampClean, PreampTransformer, PreampTube, PreampFet };

struct MicProfile
{
    const char* name;
    const char* maker;
    const char* era;
    const char* type;
    const char* pattern;
    const char* priceClass;
    const char* tagline;
    const char* traits[4];
    const char* bestFor;
    BandSpec bands[5];
    float presenceHz;
    float proxFactor;     // how strongly the proximity effect is expressed
    int   nativePreamp;   // PreampType of the mic's own electronics
    float nativeDrive;    // 0..1 amount of colour from those electronics
};

constexpr int kNumMics = 10;

inline const MicProfile kMics[kNumMics] =
{
 { "NEUMANN U 47", "NEUMANN", "VINTAGE ICON", "TUBE LARGE DIAPHRAGM", "CARDIOID / OMNI", "$$$$$",
   "The fat, round vocal sound of the 50s and 60s.",
   { "Dense, rounded low-mids with huge body",
     "Smooth rolled-off top: flattering, never harsh",
     "Tube + transformer harmonics add natural glue",
     "Gentle 5-6 kHz presence that cuts without bite" },
   "Lead vocals, kick drum, upright bass, brass",
   { {BandType::LowShelf,120.f,2.0f,0.7f}, {BandType::Peak,300.f,1.0f,0.9f}, {BandType::Peak,5500.f,1.8f,1.0f},
     {BandType::HighShelf,12000.f,-3.0f,0.7f}, {BandType::None,0.f,0.f,1.f} },
   5000.f, 1.0f, PreampTube, 0.55f },

 { "TELEFUNKEN ELA M 251E", "TELEFUNKEN", "VINTAGE ICON", "TUBE DUAL DIAPHRAGM", "CARDIOID / OMNI / FIG-8", "$$$$$",
   "Silky, expensive-sounding top end and sweet mids.",
   { "Open, silky 10-12 kHz 'air' without sizzle",
     "Tighter, more controlled lows than a U 47",
     "Sweet upper-mids flatter breathy, airy vocals",
     "Smooth tube colour with a vintage sheen" },
   "Airy pop/R&B vocals, acoustic guitar, strings",
   { {BandType::LowShelf,90.f,-0.5f,0.7f}, {BandType::Peak,250.f,-1.0f,1.0f}, {BandType::Peak,3500.f,1.2f,0.8f},
     {BandType::HighShelf,9000.f,3.0f,0.6f}, {BandType::Peak,14000.f,1.5f,0.8f} },
   4000.f, 1.0f, PreampTube, 0.50f },

 { "AKG C12", "AKG", "VINTAGE ICON", "TUBE DUAL DIAPHRAGM", "9 PATTERNS (CARDIOID HERE)", "$$$$$",
   "Crystalline shimmer around 10 kHz. Pure glass.",
   { "Glassy, crystalline lift around 9-10 kHz",
     "Very fast, detailed transient response",
     "Slimmer lows so it sits cleanly in dense mixes",
     "The classic 'shimmer' on female vocals" },
   "Female vocals, acoustic guitar, overheads, harp",
   { {BandType::LowShelf,100.f,-1.5f,0.7f}, {BandType::Peak,350.f,-0.8f,1.0f}, {BandType::Peak,4000.f,1.5f,0.9f},
     {BandType::Peak,9500.f,4.0f,0.9f}, {BandType::HighShelf,14000.f,1.5f,0.7f} },
   4500.f, 0.9f, PreampTube, 0.40f },

 { "NEUMANN U 67", "NEUMANN", "VINTAGE ICON", "TUBE LARGE DIAPHRAGM", "CARDIOID / OMNI / FIG-8", "$$$$$",
   "Punchy, thick, controlled. The studio workhorse.",
   { "Punchy, controlled lows with attitude",
     "Thick, rich lower-mids for powerful delivery",
     "Smooth top-end roll-off keeps it musical",
     "Balanced vintage voicing that needs little EQ" },
   "Powerful vocals, guitar amps, horns, drum room",
   { {BandType::LowShelf,110.f,2.0f,0.7f}, {BandType::Peak,180.f,1.0f,0.9f}, {BandType::Peak,4000.f,1.8f,1.0f},
     {BandType::HighShelf,11000.f,-1.5f,0.7f}, {BandType::None,0.f,0.f,1.f} },
   4500.f, 1.1f, PreampTube, 0.65f },

 { "SONY C-800G", "SONY", "MODERN LEGEND", "TUBE DUAL DIAPHRAGM", "CARDIOID", "$$$$$",
   "Ultra-bright, hi-fi polish. The modern pop vocal.",
   { "Extended, hi-fi top end that reaches 20 kHz",
     "Peltier-cooled tube design: ultra-low noise",
     "Scooped low-mids for a modern, polished sound",
     "Signature of chart-topping pop & hip-hop vocals" },
   "Pop/hip-hop vocals, rap, bright intimate vocals",
   { {BandType::LowShelf,120.f,-1.5f,0.7f}, {BandType::Peak,400.f,-1.0f,1.0f}, {BandType::Peak,3000.f,0.8f,0.9f},
     {BandType::Peak,8000.f,2.5f,0.9f}, {BandType::HighShelf,13000.f,4.5f,0.7f} },
   6000.f, 0.8f, PreampTube, 0.30f },

 { "NEUMANN M 149 TUBE", "NEUMANN", "MODERN CLASSIC", "TUBE DUAL DIAPHRAGM", "9 PATTERNS (CARDIOID HERE)", "$$$$",
   "Vintage lineage with modern clarity and extension.",
   { "Modern take on the M 49 / U 47 tradition",
     "Smooth, open and extended without harshness",
     "Full yet detailed, less coloured than vintage",
     "Wide, flexible tonal balance for any voice" },
   "Vocals, voice-over, piano, orchestral soloists",
   { {BandType::LowShelf,100.f,1.0f,0.7f}, {BandType::Peak,4000.f,1.0f,0.9f}, {BandType::HighShelf,10000.f,2.0f,0.7f},
     {BandType::None,0.f,0.f,1.f}, {BandType::None,0.f,0.f,1.f} },
   5000.f, 1.0f, PreampTube, 0.35f },

 { "BRAUNER VM1", "BRAUNER", "MODERN CLASSIC", "TUBE LARGE DIAPHRAGM", "9 PATTERNS (CARDIOID HERE)", "$$$$",
   "Hand-built depth and transparent, silky detail.",
   { "Exceptional detail and depth: 'inside the vocal'",
     "Silky highs with no sizzle or edge",
     "Very even tonal balance across the spectrum",
     "Hand-built German tube design, high-end clarity" },
   "Intimate vocals, acoustic instruments, ensembles",
   { {BandType::LowShelf,90.f,1.0f,0.7f}, {BandType::Peak,500.f,-0.7f,1.0f}, {BandType::Peak,3000.f,1.0f,0.8f},
     {BandType::HighShelf,11000.f,2.5f,0.6f}, {BandType::None,0.f,0.f,1.f} },
   4000.f, 0.9f, PreampTube, 0.30f },

 { "MANLEY REFERENCE", "MANLEY", "MODERN CLASSIC", "TUBE LARGE DIAPHRAGM", "CARDIOID", "$$$",
   "Warm, flattering and smooth. A studio favourite.",
   { "Warm, flattering low-mid weight",
     "Smooth upper-mids that never get brittle",
     "Gentle air lift for a polished finish",
     "Tube electronics give gentle, musical colour" },
   "Vocals, voice-over, acoustic guitar, piano",
   { {BandType::LowShelf,100.f,1.5f,0.7f}, {BandType::Peak,250.f,0.8f,0.9f}, {BandType::Peak,6000.f,1.2f,0.9f},
     {BandType::HighShelf,12000.f,1.5f,0.7f}, {BandType::None,0.f,0.f,1.f} },
   5000.f, 1.0f, PreampTube, 0.40f },

 { "NEUMANN U 87 Ai", "NEUMANN", "STUDIO STANDARD", "FET LARGE DIAPHRAGM", "CARDIOID / OMNI / FIG-8", "$$$",
   "The studio standard. Clear, present, mix-ready.",
   { "Tight, controlled lows with clear upper-mids",
     "Presence lift at 5-10 kHz for intelligibility",
     "FET-style electronics with an output transformer",
     "Sits in a mix with minimal EQ: the safe choice" },
   "Everything: vocals, VO, acoustic, drums, strings",
   { {BandType::Peak,160.f,1.0f,0.9f}, {BandType::Peak,6000.f,2.5f,1.0f}, {BandType::Peak,10000.f,2.0f,1.0f},
     {BandType::LowShelf,70.f,-1.0f,0.7f}, {BandType::None,0.f,0.f,1.f} },
   5500.f, 1.0f, PreampTransformer, 0.45f },

 { "ROYER R-121", "ROYER", "MODERN CLASSIC", "PASSIVE RIBBON", "FIGURE-8 (RIBBON)", "$$$",
   "Dark, silky and natural. Ribbon smoothness.",
   { "Passive ribbon: natural, dark, silky top end",
     "Strong proximity effect: big, intimate low end",
     "Smooth high roll-off tames harsh sources",
     "Handles loud sources with a 'photographic' tone" },
   "Guitar amps, brass, bright vocals, drum room",
   { {BandType::LowShelf,150.f,2.5f,0.6f}, {BandType::Peak,300.f,1.0f,0.9f}, {BandType::Peak,3000.f,-1.0f,0.9f},
     {BandType::HighShelf,8000.f,-4.5f,0.7f}, {BandType::HighShelf,16000.f,-3.0f,0.7f} },
   3500.f, 1.6f, PreampTransformer, 0.50f },
};
} // namespace r5t
