/**
 * @file ClawPagedGeometry.hpp
 * @brief C++ wrapper for renderer-independent paged geometry.
 */

#ifndef ClawPagedGeometry_hpp__
#define ClawPagedGeometry_hpp__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <WorkphoneGraphics/workphone_graphics_paged_geometry.h>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class GeometryPage
         * @brief C++ wrapper around wp_geometry_page.
         *
         * Represents a single page of geometry within the paged geometry system.
         * A page is a spatial region where geometry is batched and loaded/unloaded.
         */
        class WPGraphics_API GeometryPage
        {
        public:
            /**
             * @brief Constructs a GeometryPage from an existing native handle.
             * @param page Native page handle (may be nullptr).
             */
            explicit GeometryPage( wp_geometry_page *page = nullptr );

            /** @brief Destructor. */
            ~GeometryPage();

            /** @brief Returns the underlying native handle. */
            wp_geometry_page *getNative() const;

            /** @brief Returns the page data for custom use. */
            void *getData() const;

            /** @brief Returns the world-space center of this page. */
            Vector3F getCenter() const;

            /** @brief Returns true if the page is currently loaded. */
            bool isLoaded() const;

            /** @brief Returns true if the page is visible from the camera. */
            bool isVisible() const;

            /** @brief Returns the page's query flag for rendering. */
            u32 getQueryFlag() const;

            /**
             * @brief Returns the bounding box of all entities added to this page.
             * @return An AABB in world space.
             */
            AABB3<real_Num> getBoundingBox() const;

            /**
             * @brief Adds an entity to this page.
             * @param entity Custom entity pointer.
             * @param position World position of the entity.
             * @param rotation World rotation of the entity (w, x, y, z).
             * @param scale Uniform scale of the entity.
             * @param color Color tint for the entity.
             * @param localBounds Optional local-space bounding box for culling.
             */
            void addEntity( void *entity, const Vector3F &position, const QuaternionF &rotation,
                            const Vector3F &scale, const wp_paged_geometry_color &color,
                            const wp_paged_geometry_aabb *localBounds = nullptr );

            /** @brief Conversion operator for convenient null checks. */
            operator bool() const;

        private:
            wp_geometry_page *m_page;
        };

        /**
         * @class GeometryPageManager
         * @brief C++ wrapper around wp_geometry_page_manager.
         *
         * Manages pages for a single detail level (LOD tier) in the paged geometry
         * system.  Each detail level has its own page manager with independent
         * range and transition settings.
         */
        class WPGraphics_API GeometryPageManager
        {
        public:
            /**
             * @brief Constructs a GeometryPageManager from an existing native handle.
             * @param manager Native manager handle (may be nullptr).
             */
            explicit GeometryPageManager( wp_geometry_page_manager *manager = nullptr );

            /** @brief Destructor. */
            ~GeometryPageManager();

            /** @brief Returns the underlying native handle. */
            wp_geometry_page_manager *getNative() const;

            /** @brief Returns the near distance where this detail level takes over. */
            f32 getNearRange() const;

            /** @brief Returns the far distance beyond which pages are unloaded. */
            f32 getFarRange() const;

            /** @brief Returns the transition (fade) length between detail levels. */
            f32 getTransitionLength() const;

            /**
             * @brief Sets the caching behavior for this detail level.
             * @param maxCacheIntervalMs Maximum time a page can stay loaded after going out of view.
             * @param inactivePageLifeMs Time before an inactive page is considered for unloading.
             */
            void setCacheSpeed( u32 maxCacheIntervalMs, u32 inactivePageLifeMs );

            /** @brief Returns the number of currently loaded pages. */
            u32 getLoadedCount() const;

            /** @brief Returns the number of pages being asynchronously loaded. */
            u32 getPendingCount() const;

            /** @brief Conversion operator for convenient null checks. */
            operator bool() const;

        private:
            wp_geometry_page_manager *m_manager;
        };

        /**
         * @class PagedGeometry
         * @brief C++ wrapper around wp_paged_geometry.
         *
         * Provides a renderer-independent paging system for large worlds.  Geometry
         * is partitioned into pages (spatial tiles) which are loaded and unloaded
         * based on camera distance.  Multiple detail levels (LOD tiers) are supported,
         * each managed by a separate GeometryPageManager.
         *
         * The page callbacks are provided by a subclass of PageLoader.
         */
        class WPGraphics_API PagedGeometry
        {
        public:
            /**
             * @brief Creates a PagedGeometry instance.
             * @param camera The camera used to determine which pages are visible.
             * @param pageSize The width and height of each page in world units.
             */
            PagedGeometry( wp_camera *camera, f32 pageSize );

            /** @brief Destructor. Removes all detail levels and the geometry. */
            ~PagedGeometry();

            // --- Configuration ---

            /** @brief Returns the current page size in world units. */
            f32 getPageSize() const;

            /**
             * @brief Sets the page size.  Fails if detail levels have already been added.
             * @return true on success, false if rejected.
             */
            bool setPageSize( f32 pageSize );

            /**
             * @brief Sets the rectangular bounds of the world (pages outside are never loaded).
             * @param left Left edge in world units.
             * @param top Top edge in world units.
             * @param right Right edge in world units.
             * @param bottom Bottom edge in world units.
             * @return true on success, false if rejected.
             */
            bool setBounds( f32 left, f32 top, f32 right, f32 bottom );

            /** @brief Returns the world bounds. */
            wp_paged_geometry_bounds getBounds() const;

            /** @brief Enables infinite world mode (no bounds checking). */
            void setInfinite();

            /** @brief Sets the active camera. */
            void setCamera( wp_camera *camera );

            /** @brief Returns the active camera. */
            wp_camera *getCamera() const;

            // --- Detail levels ---

            /**
             * @brief Adds a detail (LOD) level.
             * @param maxRange Maximum distance at which this detail level is used.
             * @param transitionLength Fade/blend distance to the next farther level.
             * @param queryFlag Render query flag for pages at this level.
             * @return A manager for this detail level.
             * @note Detail levels must be added from nearest to farthest.
             */
            GeometryPageManager addDetailLevel( f32 maxRange, f32 transitionLength = 0.0f,
                                                u32 queryFlag = 0 );

            /** @brief Removes all detail levels. */
            void removeDetailLevels();

            /** @brief Returns the number of registered detail levels. */
            u32 getDetailLevelCount() const;

            // --- Page loader ---

            /**
             * @brief Sets the page loader callbacks.
             * @param callbacks Pointer to a callback struct (must remain valid until next call or destruction).
             * @param loaderData Opaque user data passed to callbacks.
             */
            void setPageLoader( const wp_page_loader_callbacks *callbacks, void *loaderData );

            // --- Update ---

            /**
             * @brief Updates the paging system.  Call once per frame.
             * @param deltaMs Time elapsed since the last call in milliseconds.
             */
            void update( u32 deltaMs );

            // --- Reload / preload ---

            /** @brief Unloads all pages and reloads them immediately. */
            void reload();

            /** @brief Reloads the page containing the given point. */
            void reloadPage( const Vector3F &point );

            /** @brief Reloads all pages within the given radius of the center point. */
            void reloadPagesRadius( const Vector3F &center, f32 radius );

            /** @brief Reloads all pages intersecting the given rectangular bounds. */
            void reloadPagesBounds( const wp_paged_geometry_bounds &bounds );

            /** @brief Preloads pages in the given area. */
            void preload( const wp_paged_geometry_bounds &bounds );

            /** @brief Cancels any pending preloads. */
            void resetPreloaded();

            // --- Visibility ---

            /** @brief Enables or disables rendering of all paged geometry. */
            void setVisible( bool visible );

            /** @brief Returns the current visibility state. */
            bool isVisible() const;

            /** @brief Returns the underlying native handle. */
            wp_paged_geometry *getNative() const;

            /** @brief Conversion operator for convenient null checks. */
            operator bool() const;

        private:
            wp_paged_geometry *m_geometry;
        };

    }  // namespace render
}  // namespace workphone

#endif  // ClawPagedGeometry_hpp__
