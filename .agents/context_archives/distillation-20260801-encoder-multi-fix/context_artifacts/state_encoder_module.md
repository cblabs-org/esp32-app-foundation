# State: Encoder Module Multi-Encoder Architecture

## Core Structures
The core configuration is in `EncoderConfig.h`.

```cpp
struct BoardEncoder {
    const char*        id;
    int8_t             pinA;
    int8_t             pinB;
    uint8_t            stepsPerDetent;
    bool               reversed;
    EncoderAccelConfig accel;
};
```

## Runtime State Bundle
Instead of flat fields in `EncoderModule`, each encoder has a bundled state tracking its handles, accumulators, and diagnostics.

```cpp
struct EncoderState {
    BoardEncoder config{};
    pcnt_unit_handle_t    pcntUnit = nullptr;
    pcnt_channel_handle_t pcntChanA = nullptr;
    pcnt_channel_handle_t pcntChanB = nullptr;

    std::atomic<int32_t> rawDelta{0};
    std::atomic<int32_t> accelDelta{0};
    int32_t fractionalAccum = 0;
    int prevCount = 0; // Delta-from-previous pattern

    std::atomic<uint32_t> lastMovementMs{0};
    std::atomic<int32_t>  position{0};
};
```

## Internal Storage
`EncoderModule.hpp` holds a static array bounded by the hardware SOC constant, ensuring deterministic memory size.

```cpp
#ifdef SOC_PCNT_UNITS_PER_GROUP
    static constexpr size_t MAX_ENCODERS = SOC_PCNT_UNITS_PER_GROUP;
#else
    static constexpr size_t MAX_ENCODERS = 4;
#endif

EncoderState _encoders[MAX_ENCODERS];
size_t       _encoderCount = 0;
```

## API
Consumers extract deltas for specific encoders via index.

```cpp
int32_t consumeDeltaAt(size_t encoderIndex, EncoderDelta mode = EncoderDelta::RAW);
int32_t consumeDelta(EncoderDelta mode = EncoderDelta::RAW); // Wrapper for consumeDeltaAt(0)
```
