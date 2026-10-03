# Skybox panorama navigation

`SkyboxPanoramaGenerator` creates a grid of discrete panorama stations. The viewer can freely
look around at a station and jump to connected stations, matching the navigation model used by
Street View.

## Scene setup

Put `Skybox` and `SkyboxPanoramaGenerator` on the same actor, then configure the generator:

```cpp
#include <Workphone/Workphone.hpp>

auto panoramaActor = sceneManager->createActor();
panoramaActor->setName( "Panorama World" );

// Two independently-materialed skyboxes allow a real panorama-to-panorama dissolve.
auto skyboxA = panoramaActor->addComponent<scene::Skybox>();
auto skyboxB = panoramaActor->addComponent<scene::Skybox>();
auto panoramas = panoramaActor->addComponent<scene::SkyboxPanoramaGenerator>();

panoramas->setCameraActor( cameraActor );
panoramas->setPanoramaDirectory( "Panoramas/City" );
panoramas->setFilePrefix( "street" );
panoramas->setFileExtension( ".jpg" );

panoramas->setGridColumns( 12 );
panoramas->setGridRows( 8 );
panoramas->setSpacingX( 8.0f );   // metres between images from west to east
panoramas->setSpacingZ( 10.0f );  // metres between images from north to south
panoramas->setEyeHeight( 1.7f );

panoramas->setMaximumLinkDistance( 11.0f );
panoramas->setSwitchDistance( 3.0f );
panoramas->setSnapCamera( true );

// Street View-style transition controls.
panoramas->setTransitionEnabled( true );
panoramas->setTransitionDuration( 0.7f );
panoramas->setTransitionEasing(
    scene::SkyboxPanoramaGenerator::TransitionEasing::SmoothStep );
panoramas->setInterpolateCamera( true );
panoramas->setTransitionArcHeight( 0.15f );
panoramas->setTransitionFovOffset( -5.0f );
panoramas->setPanoramaBlendMode(
    scene::SkyboxPanoramaGenerator::PanoramaBlendMode::CrossFade );
panoramas->setCrossFadeStart( 0.1f );
panoramas->setCrossFadeEnd( 0.9f );

panoramas->rebuildPanoramas();

// Bind these calls to arrows, WASD, a gamepad, or clickable navigation markers.
panoramas->moveForward();
panoramas->moveBackward();
panoramas->moveLeft();
panoramas->moveRight();

scene->addActor( panoramaActor );
```

For a camera that physically walks through the scene, disable snapping and enable automatic
nearest-station switching:

```cpp
panoramas->setSnapCamera( false );
panoramas->setFollowCamera( true );
```

## Image layout

The default configuration loads six cubemap faces per station:

```text
Panoramas/City/street_c0_r0_front.jpg
Panoramas/City/street_c0_r0_back.jpg
Panoramas/City/street_c0_r0_left.jpg
Panoramas/City/street_c0_r0_right.jpg
Panoramas/City/street_c0_r0_up.jpg
Panoramas/City/street_c0_r0_down.jpg
Panoramas/City/street_c1_r0_front.jpg
...
```

All controls also appear in the component property editor. A value of `0` for Automatic Switch
Distance or Maximum Link Distance selects a spacing-derived default.

## Transition options

- **Transition Duration** controls the travel time in seconds.
- **Transition Easing** supports Linear, SmoothStep, Ease In, Ease Out and cubic Ease In/Out.
- **Interpolate Camera** enables spatial movement between panorama stations.
- **Transition Arc Height** adds a small parabolic lift to the camera path.
- **Transition FOV Offset** adds a midpoint FOV pulse; negative values produce a brief zoom-in.
- **Panorama Blend Mode** selects a timed image swap or a true two-skybox cross-fade.
- **Image Switch Point** controls when the timed fallback changes image.
- **Cross Fade Start/End** control which part of the transition contains the dissolve.
- **Cross Fade Distance Offset** places the incoming skybox slightly inside the outgoing one to
  keep transparent draw ordering deterministic.
- **Allow Transition Interruption** controls whether a new navigation request can replace one
  already in progress.

Cross-fading requires two `Skybox` components on the same actor. They must not share a `Material`
component, because the generator needs independent opacity values. With only one skybox, CrossFade
automatically uses the Image Switch Point fallback.

The component maps already captured cube faces to stations. Baking the scene into images is a
renderer operation; the existing realtime `Cubemap` component can provide that capture source,
but exporting its GPU texture to six image files needs to be implemented by the active renderer.
