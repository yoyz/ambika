# Ambika Softsynth

Port of the [Ambika](https://github.com/pichenettes/ambika) hybrid polysynth
to desktop C++. The hardware runs on ATmega644P (controller) + 6× ATmega328P
(voicecards). This softsynth replicates the voicecard synthesis engine in
portable C++ with a digital TPT SVF filter, adds LV2 and VST3 plugin wrappers,
and includes 208 built-in patches from the original factory bank.

```bash
make && make test  # standalone demo + 50 tests
```
