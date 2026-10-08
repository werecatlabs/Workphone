# VehicleAdvanced audio assets

These files are bundled locally and copied beside the executable by CMake.
No downloads or accounts are required when building or playing the sample.

## Engine recordings

`engine_0.wav` through `engine_5.wav`: **racing car engine sound loops** by
**domasx2**, from [OpenGameArt](https://opengameart.org/content/racing-car-engine-sound-loops).
Licence: [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/).
Original files: `loop_0.wav`, `loop_1_0.wav` through `loop_5_0.wav`.
The author remade these from a public-domain recording. The files have been
level-normalized here and remain mono PCM WAVs. These are generic racing loops,
not recordings from F1 2012.

## Tyre squeal recording

`tyre_squeal.wav`: **Car tire squeal skid loop**, recording by **audible-edge
(Tom Haigh)**, loop prepared and submitted by **qubodup**, from
[OpenGameArt](https://opengameart.org/content/car-tire-squeal-skid-loop).
Licence: [Creative Commons Attribution 3.0 Unported](https://creativecommons.org/licenses/by/3.0/).
Original file: `tires_squal_loop.wav`. Converted from 24-bit to 16-bit PCM and
level-normalized; its 96 kHz sample rate and three-second loop are preserved.
Credit these authors and retain the source and licence links when redistributing.
No endorsement by the authors is implied.

## Tyre rolling loop

`tyre_roll.wav`: original deterministic filtered-noise loop generated for this
sample by `prepare_assets.py`; dedicated to the public domain under
[CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/).

`prepare_assets.py` reproduces the conversions and generated rolling loop using
only the Python standard library. Run it only when regenerating these assets.
