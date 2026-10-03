/**
 * @file ClawPagedGeometry.cpp
 * @brief C++ wrapper implementation for renderer-independent paged geometry.
 */

#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/PagedGeometry/ClawPagedGeometry.hpp>
#include <WPGraphics/ClawCamera.hpp>

namespace workphone
{
    namespace render
    {

        // =============================================================================
        // GeometryPage
        // =============================================================================

        GeometryPage::GeometryPage( wp_geometry_page *page ) : m_page( page )
        {
        }

        GeometryPage::~GeometryPage()
        {
            m_page = nullptr;
        }

        wp_geometry_page *GeometryPage::getNative() const
        {
            return m_page;
        }

        void *GeometryPage::getData() const
        {
            return wp_geometry_page_get_data( m_page );
        }

        Vector3F GeometryPage::getCenter() const
        {
            const wp_vec3f center = wp_geometry_page_get_center( m_page );
            return Vector3( center.x, center.y, center.z );
        }

        bool GeometryPage::isLoaded() const
        {
            return wp_geometry_page_is_loaded( m_page ) != 0;
        }

        bool GeometryPage::isVisible() const
        {
            return wp_geometry_page_is_visible( m_page ) != 0;
        }

        u32 GeometryPage::getQueryFlag() const
        {
            return wp_geometry_page_get_query_flag( m_page );
        }

        AABB3<real_Num> GeometryPage::getBoundingBox() const
        {
            const wp_paged_geometry_aabb aabb = wp_geometry_page_get_bounding_box( m_page );
            return AABB3<real_Num>( Vector3F( aabb.min.x, aabb.min.y, aabb.min.z ),
                                    Vector3F( aabb.max.x, aabb.max.y, aabb.max.z ) );
        }

        void GeometryPage::addEntity( void *entity, const Vector3F &position,
                                      const QuaternionF &rotation, const Vector3F &scale,
                                      const wp_paged_geometry_color &color,
                                      const wp_paged_geometry_aabb *localBounds )
        {
            const wp_vec3f pos = { position.x, position.y, position.z };
            const wp_paged_geometry_quat rot = { rotation.w, rotation.x, rotation.y, rotation.z };
            const wp_vec3f scl = { scale.x, scale.y, scale.z };
            wp_geometry_page_add_entity( m_page, entity, pos, rot, scl, color, localBounds );
        }

        GeometryPage::operator bool() const
        {
            return m_page != nullptr;
        }

        // =============================================================================
        // GeometryPageManager
        // =============================================================================

        GeometryPageManager::GeometryPageManager( wp_geometry_page_manager *manager ) :
            m_manager( manager )
        {
        }

        GeometryPageManager::~GeometryPageManager()
        {
            m_manager = nullptr;
        }

        wp_geometry_page_manager *GeometryPageManager::getNative() const
        {
            return m_manager;
        }

        f32 GeometryPageManager::getNearRange() const
        {
            return wp_geometry_page_manager_get_near_range( m_manager );
        }

        f32 GeometryPageManager::getFarRange() const
        {
            return wp_geometry_page_manager_get_far_range( m_manager );
        }

        f32 GeometryPageManager::getTransitionLength() const
        {
            return wp_geometry_page_manager_get_transition( m_manager );
        }

        void GeometryPageManager::setCacheSpeed( u32 maxCacheIntervalMs, u32 inactivePageLifeMs )
        {
            wp_geometry_page_manager_set_cache_speed( m_manager, maxCacheIntervalMs,
                                                      inactivePageLifeMs );
        }

        u32 GeometryPageManager::getLoadedCount() const
        {
            return wp_geometry_page_manager_get_loaded_count( m_manager );
        }

        u32 GeometryPageManager::getPendingCount() const
        {
            return wp_geometry_page_manager_get_pending_count( m_manager );
        }

        GeometryPageManager::operator bool() const
        {
            return m_manager != nullptr;
        }

        // =============================================================================
        // PagedGeometry
        // =============================================================================

        PagedGeometry::PagedGeometry( wp_camera *camera, f32 pageSize ) :
            m_geometry( wp_paged_geometry_create( camera, pageSize ) )
        {
        }

        PagedGeometry::~PagedGeometry()
        {
            if( m_geometry != nullptr )
            {
                wp_paged_geometry_destroy( m_geometry );
                m_geometry = nullptr;
            }
        }

        f32 PagedGeometry::getPageSize() const
        {
            return wp_paged_geometry_get_page_size( m_geometry );
        }

        bool PagedGeometry::setPageSize( f32 pageSize )
        {
            return wp_paged_geometry_set_page_size( m_geometry, pageSize ) != 0;
        }

        bool PagedGeometry::setBounds( f32 left, f32 top, f32 right, f32 bottom )
        {
            const wp_paged_geometry_bounds bounds = { left, top, right, bottom };
            return wp_paged_geometry_set_bounds( m_geometry, bounds ) != 0;
        }

        wp_paged_geometry_bounds PagedGeometry::getBounds() const
        {
            return wp_paged_geometry_get_bounds( m_geometry );
        }

        void PagedGeometry::setInfinite()
        {
            wp_paged_geometry_set_infinite( m_geometry );
        }

        void PagedGeometry::setCamera( wp_camera *camera )
        {
            wp_paged_geometry_set_camera( m_geometry, camera );
        }

        wp_camera *PagedGeometry::getCamera() const
        {
            return wp_paged_geometry_get_camera( m_geometry );
        }

        GeometryPageManager PagedGeometry::addDetailLevel( f32 maxRange, f32 transitionLength,
                                                           u32 queryFlag )
        {
            return GeometryPageManager( wp_paged_geometry_add_detail_level(
                m_geometry, maxRange, transitionLength, nullptr, nullptr, nullptr, queryFlag ) );
        }

        void PagedGeometry::removeDetailLevels()
        {
            wp_paged_geometry_remove_detail_levels( m_geometry );
        }

        u32 PagedGeometry::getDetailLevelCount() const
        {
            return wp_paged_geometry_get_detail_level_count( m_geometry );
        }

        void PagedGeometry::setPageLoader( const wp_page_loader_callbacks *callbacks, void *loaderData )
        {
            wp_paged_geometry_set_page_loader( m_geometry, callbacks, loaderData );
        }

        void PagedGeometry::update( u32 deltaMs )
        {
            wp_paged_geometry_update( m_geometry, deltaMs );
        }

        void PagedGeometry::reload()
        {
            wp_paged_geometry_reload( m_geometry );
        }

        void PagedGeometry::reloadPage( const Vector3F &point )
        {
            const wp_vec3f pos = { point.x, point.y, point.z };
            wp_paged_geometry_reload_page( m_geometry, pos );
        }

        void PagedGeometry::reloadPagesRadius( const Vector3F &center, f32 radius )
        {
            const wp_vec3f pos = { center.x, center.y, center.z };
            wp_paged_geometry_reload_pages_radius( m_geometry, pos, radius );
        }

        void PagedGeometry::reloadPagesBounds( const wp_paged_geometry_bounds &bounds )
        {
            wp_paged_geometry_reload_pages_bounds( m_geometry, bounds );
        }

        void PagedGeometry::preload( const wp_paged_geometry_bounds &bounds )
        {
            wp_paged_geometry_preload( m_geometry, bounds );
        }

        void PagedGeometry::resetPreloaded()
        {
            wp_paged_geometry_reset_preloaded( m_geometry );
        }

        void PagedGeometry::setVisible( bool visible )
        {
            wp_paged_geometry_set_visible( m_geometry, visible ? 1 : 0 );
        }

        bool PagedGeometry::isVisible() const
        {
            return wp_paged_geometry_is_visible( m_geometry ) != 0;
        }

        wp_paged_geometry *PagedGeometry::getNative() const
        {
            return m_geometry;
        }

        PagedGeometry::operator bool() const
        {
            return m_geometry != nullptr;
        }

    }  // namespace render
}  // namespace workphone
