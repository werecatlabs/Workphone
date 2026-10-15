// Compile the production sources headlessly, bypassing the legacy global PCH flag.
// This wrapper is used only by the standalone regression target.
#undef WP_USE_PRECOMPILED_HEADERS
#define WP_USE_PRECOMPILED_HEADERS 0
#include "../../../Engine/cpp/Source/Workphone/Vehicle/CarVisual.cpp"
#include "../../../Engine/cpp/Source/Workphone/Vehicle/CarVisualAssets.cpp"
