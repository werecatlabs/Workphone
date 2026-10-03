/**
 * @file WPCore.hpp
 * @brief Core initialization class for the Workphone game engine
 * @author Workphone Development Team
 * @date 2025
 * @version 1.0
 */

/**
 * @mainpage Workphone Game Engine Documentation
 *
 * @section intro_sec Introduction
 *
 * The Workphone game engine is a comprehensive C++ game development framework
 * designed for creating high-performance interactive applications and games.
 *
 * @section features_sec Key Features
 * - Cross-platform support
 * - Modern C++17 architecture
 * - Modular component system
 * - High-performance rendering
 * - Physics simulation
 * - Audio processing
 * - Asset management
 *
 * @section getting_started_sec Getting Started
 *
 * To initialize the engine, create an instance of WPCore and call the appropriate
 * load methods to set up the engine subsystems.
 *
 * @code{.cpp}
 * auto core = std::make_shared<workphone::WPCore>();
 * core->load(nullptr);
 * @endcode
 */

#ifndef __WPCore_H_
#define __WPCore_H_

#include <Workphone/WorkphoneAutolink.hpp>
#include <Workphone/WorkphoneEnums.hpp>
#include <Workphone/WorkphoneHeaders.hpp>
#include <Workphone/WorkphonePlugin.hpp>

#endif  // __WPCore_H_
