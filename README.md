# RODE5TUNE - Microphone Character Emulator (VST3 / AU)

Turns your **RODE NT1 5th Generation** into one of 10 legendary microphones.
Black, retro-modern interface with VU meters, a live response scope and tooltips on every control.

## Get the plugin (no coding needed)
1. Create a new GitHub repository and upload the contents of this folder (keep `.github/workflows/build.yml`).
2. Open the **Actions** tab -> **Build Rode5tune** -> **Run workflow**.
3. When finished (about 10 min), open the run and download the artifacts:
   `Rode5tune-Windows`, `Rode5tune-macOS`, `Rode5tune-Linux`.
4. To get a downloadable Release page automatically, push a tag: `git tag v1.0.0 && git push --tags`.

## Install
| OS | Copy to |
|----|---------|
| Windows | `Rode5tune.vst3` folder -> `C:\Program Files\Common Files\VST3\` |
| macOS | `Rode5tune.vst3` -> `/Library/Audio/Plug-Ins/VST3/`  and `Rode5tune.component` -> `/Library/Audio/Plug-Ins/Components/`. Unsigned build: run `xattr -cr <file>` once. |
| Linux | `Rode5tune.vst3` -> `~/.vst3/` |

Then rescan plugins in your DAW. A standalone app is included for quick testing.

## Best-practice setup with the NT1 5th Gen
* Open **RODE Central** and switch the NT1's on-board DSP (Aphex processing / compressor / noise gate / high-pass) **off**, so the plugin works on the clean signal.
* Set the input gain so peaks land around -12 dBFS. Then adjust TRIM until the INPUT meter sits near 0 VU.
* Keep the plugin **first** in the insert chain (before compression/EQ).
* Compare mics at equal loudness using OUTPUT.

## Signal chain
`Trim -> Low Cut -> NT1 Neutralizer -> Mic voicing (5 bands) -> Proximity -> Presence -> Air -> Preamp saturation (2x oversampled) -> DC block -> De-esser -> Output -> Dry/Wet`

## Build locally
```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
Requires CMake 3.22+ and a C++17 compiler. JUCE 8.0.6 is downloaded automatically.

## Honest limits
The mic curves are voicings based on the documented sonic character of each mic, not measurements of the real units, and no plugin can change a capsule's polar pattern or off-axis behaviour. See `docs/NT1_ANALYSIS.md` for how to refine the profiles with your own A/B recordings.

## Licensing / trademarks
Built with JUCE (AGPLv3 or commercial licence - a commercial closed-source release needs a JUCE licence). Microphone names are trademarks of their respective owners; they are used only to describe the sonic target. Rode5tune is not affiliated with or endorsed by them. Review this before selling the plugin.
