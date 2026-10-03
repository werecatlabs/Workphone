#ifndef _OgreVulkanBuildSettings_H_
#define _OgreVulkanBuildSettings_H_

#if defined( __ANDROID__ )
#    define OGRE_VULKAN_WINDOW_ANDROID 1
#endif

#ifndef OGRE_VULKAN_MAX_NUM_BOUND_DESCRIPTOR_SETS
#    define OGRE_VULKAN_MAX_NUM_BOUND_DESCRIPTOR_SETS 8
#endif

#endif
