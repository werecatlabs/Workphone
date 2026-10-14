#ifndef UnitTests_h__
#define UnitTests_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <TypeManagerFixture.hpp>
#include "TestGuard.hpp"

namespace workphone
{

    /**
     * @file UnitTests.hpp
     * @brief Declarations for test environment setup and helpers used by the unit test suite.
     */

    // Forward declarations for classes used by the UnitTests helper.
    class WorkphonePlugin;
    class UnitTestsFixture;

    /**
     * @brief Central helper for configuring and controlling the unit-test runtime.
     *
     * UnitTests provides static helpers used by the tests to initialize the engine
     * subsystems (plugins, graphics, game logic, factories), to create test tasks,
     * and to manage a shared test fixture and TypeManager instance used across tests.
     *
     * All members are static because the class models a global test environment,
     * not per-instance state.
     */
    class UnitTests
    {
    public:
        /**
         * @brief Load and initialise application plugins required by tests.
         *
         * This will typically parse the configured plugins file and register/load
         * plugin modules into the test runtime so other setup steps can rely on them.
         */
        static void setupPlugins();

        /**
         * @brief Initialise the default test runtime environment.
         *
         * Sets up core subsystems required for most unit tests (memory, logging,
         * basic services). Call this before tests that require a running engine core.
         */
        static void setupDefault();

        /**
         * @brief Initialise graphics subsystems used by rendering tests.
         *
         * This configures any minimal graphics context / renderer stubs required
         * for tests that exercise rendering or graphics-dependent code paths.
         */
        static void setupGraphics();

        /**
         * @brief Initialise game-specific subsystems for gameplay tests.
         *
         * Sets up game logic, entity systems, or other systems specific to
         * the game layer used by unit tests.
         */
        static void setupGame();

        /**
         * @brief Register or create factory objects required by tests.
         *
         * Factories may be required to create shared objects during test setup.
         */
        static void setupFactories();

        /**
         * @brief Create background or scheduled tasks used by tests.
         *
         * This will enqueue or create any asynchronous tasks that tests rely on
         * (for example simulated job system work or periodic maintenance tasks).
         */
        static void createTasks();

        /**
         * @brief Tear down the default test runtime and release resources.
         *
         * Should be called after tests finish to ensure a clean shutdown of systems
         * initialised by @c setupDefault().
         */
        static void destroyDefault();

        /**
         * @brief Get the path to the plugins configuration file used by tests.
         * @return Current plugins file path as a String.
         */
        static String &getPluginsFilePath();

        /**
         * @brief Set the path to the plugins configuration file to use for tests.
         * @param pluginsFilePath Path to the plugins file.
         */
        static void setPluginsFilePath( const String &pluginsFilePath );

        /** @brief Detect whether the runtime is in a headless graphics configuration (no real render window). */
        static bool isHeadlessGraphicsMode();

        /** @brief Detect whether the runtime has a working sound backend. */
        static bool isHeadlessSoundMode();

        /**
         * @brief Perform a number of update iterations on the default runtime.
         * @param iterations Number of update cycles to perform.
         *
         * Useful for advancing the test runtime (processing tasks, timers,
         * or other per-frame work) in a deterministic way during tests.
         */
        static void updateDefault( u32 iterations );

        /**
         * @brief Retrieve the active TestFixture instance used by tests.
         * @return Pointer to the current TestFixture, or nullptr if none is set.
         */
        static UnitTestsFixture *getFixture();

        /**
         * @brief Set the TestFixture instance to be used by tests.
         * @param fixture Pointer to the fixture instance.
         */
        static void setFixture( UnitTestsFixture *fixture );

        /** @brief Shared TypeManager instance used across unit tests. */
        static TypeManager *sTypeManager;

    protected:
        /** @brief Smart pointer to the core/plugin manager used by the test runtime. */
        static SmartPtr<WorkphonePlugin> m_plugin;

        /** @brief Pointer to the currently registered TestFixture (if any). */
        static UnitTestsFixture *m_fixture;

    };

}  // namespace workphone

#endif  // UnitTests_h__
