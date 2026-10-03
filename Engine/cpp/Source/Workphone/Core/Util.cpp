#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/Util.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/UI/IUIImage.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/Interface/UI/IUIMenu.hpp>
#include <Workphone/Interface/UI/IUITreeNode.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

#if defined WP_PLATFORM_WIN32
#    include <windows.h>
#    include <xmmintrin.h>
#endif

namespace workphone
{

    const String Util::resourceStr = String( "resource" );
    const String Util::resourceTypeStr = String( "resourceType" );
    const String Util::materialStr = String( "Material" );
    const String Util::textureStr = String( "Texture" );
    const String Util::componentStr = String( "Component" );
    const String Util::noneStr = String( "None" );
    const String Util::attributeNameStr = String( "click" );
    const String Util::defaultValue = String( "true" );
    const String Util::buttonTypeStr = String( "button" );
    const String Util::enumTypeStr = String( "enum" );
    const String Util::defaultType = String( "string" );
    const String Util::nullStr = String( "null" );
    const String Util::colourStr = String( "colour" );
    const String Util::colouriStr = String( "colouri" );
    const String Util::boolStr = String( "bool" );
    const String Util::intStr = String( "int" );
    const String Util::floatStr = String( "float" );
    const String Util::doubleStr = String( "double" );
    const String Util::vector2Str = String( "vector2" );
    const String Util::vector2fStr = String( "vector2f" );
    const String Util::vector2dStr = String( "vector2d" );
    const String Util::vector2iStr = String( "vector2i" );
    const String Util::vector3Str = String( "vector3" );
    const String Util::vector3fStr = String( "vector3f" );
    const String Util::vector3dStr = String( "vector3d" );
    const String Util::vector3iStr = String( "vector3i" );
    const String Util::quatdStr = String( "quatd" );
    const String Util::quatfStr = String( "quatf" );

    const s32 Util::compact_range = std::numeric_limits<short>::max();

    // Courtesy of William Chan and Google. 30-70% faster than memcpy in Microsoft Visual Studio 2005.
    void Util::X_aligned_memcpy_sse2( void *dest, const void *src, const size_t size )
    {
        memcpy( dest, src, size );

#ifdef WP_PLATFORM_WIN32
#    if WP_ARCH_TYPE == WP_ARCHITECTURE_32
#        if 0
        __asm {
			mov esi, src;  // src pointer
			mov edi, dest;  // dest pointer

			mov ebx, size;  // ebx is our counter
			shr ebx, 7;  // divide by 128 (8 * 128bit registers)


		loop_copy:
			prefetchnta 128[ESI];  // SSE2 prefetch
			prefetchnta 160[ESI];
			prefetchnta 192[ESI];
			prefetchnta 224[ESI];

			movdqa xmm0, 0[ESI];  // move data from src to registers
			movdqa xmm1, 16[ESI];
			movdqa xmm2, 32[ESI];
			movdqa xmm3, 48[ESI];
			movdqa xmm4, 64[ESI];
			movdqa xmm5, 80[ESI];
			movdqa xmm6, 96[ESI];
			movdqa xmm7, 112[ESI];

			movntdq 0[EDI], xmm0;  // move data from registers to dest
			movntdq 16[EDI], xmm1;
			movntdq 32[EDI], xmm2;
			movntdq 48[EDI], xmm3;
			movntdq 64[EDI], xmm4;
			movntdq 80[EDI], xmm5;
			movntdq 96[EDI], xmm6;
			movntdq 112[EDI], xmm7;

			add esi, 128;
			add edi, 128;
			dec ebx;

			jnz loop_copy;  // loop please
		loop_copy_end:
        }
#        endif
#    else
        memcpy( dest, src, size );
#    endif
#else
        memcpy( dest, src, size );
#endif
    }

    void Util::accurateSleep( [[maybe_unused]] f64 seconds )
    {
#if defined WP_PLATFORM_WIN32
        if( seconds == 0.0 )
        {
            return;
        }

        static LARGE_INTEGER s_freq = { { 0, 0 } };
        if( s_freq.QuadPart == 0 )
        {
            QueryPerformanceFrequency( &s_freq );
        }

        LARGE_INTEGER from, now;
        QueryPerformanceCounter( &from );
        auto ticks_to_wait = static_cast<s32>( static_cast<f64>( s_freq.QuadPart ) / ( 1.0 / seconds ) );
        auto done = false;
        auto ticks_passed = 0;
        auto ticks_left = 0;

        do
        {
            QueryPerformanceCounter( &now );
            ticks_passed = static_cast<s32>( now.QuadPart - from.QuadPart );
            ticks_left = ticks_to_wait - ticks_passed;

            // time wrap
            if( now.QuadPart < from.QuadPart )
            {
                done = true;
            }

            if( ticks_passed >= ticks_to_wait )
            {
                done = true;
            }

            if( !done )
            {
                Thread::yield();
            }
        } while( !done );
#endif
    }

    void Util::getDesktopResolution( [[maybe_unused]] s32 &horizontal, [[maybe_unused]] s32 &vertical )
    {
#if defined WP_PLATFORM_WIN32
        RECT desktop;
        // Get a handle to the desktop window
        const HWND hDesktop = GetDesktopWindow();
        // Get the size of screen to the variable desktop
        GetWindowRect( hDesktop, &desktop );
        // The top left corner will have coordinates (0,0)
        // and the bottom right corner will have coordinates
        // (horizontal, vertical)
        horizontal = desktop.right;
        vertical = desktop.bottom;
#endif
    }

#if defined WP_PLATFORM_WIN32
    auto Util::crossProduct( __m128 a, __m128 b ) -> __m128
    {
        return _mm_sub_ps( _mm_mul_ps( _mm_shuffle_ps( a, a, _MM_SHUFFLE( 3, 0, 2, 1 ) ),
                                       _mm_shuffle_ps( b, b, _MM_SHUFFLE( 3, 1, 0, 2 ) ) ),
                           _mm_mul_ps( _mm_shuffle_ps( a, a, _MM_SHUFFLE( 3, 1, 0, 2 ) ),
                                       _mm_shuffle_ps( b, b, _MM_SHUFFLE( 3, 0, 2, 1 ) ) ) );
    }
#endif

    auto Util::compactFloat( f64 input, s32 range ) -> s16
    {
        return static_cast<s16>( Math<f64>::round( input * compact_range / range ) );
    }

    auto Util::expandToFloat( s16 input, s32 range ) -> f64
    {
        return static_cast<f64>( input ) * range / compact_range;
    }

    void Util::convertBetweenBGRAandRGBA( u8 *input, s32 pixel_width, s32 pixel_height, u8 *output )
    {
        auto offset = 0;

        for( s32 y = 0; y < pixel_height; y++ )
        {
            for( s32 x = 0; x < pixel_width; x++ )
            {
                output[offset] = input[offset + 2];
                output[offset + 1] = input[offset + 1];
                output[offset + 2] = input[offset];
                output[offset + 3] = input[offset + 3];

                offset += 4;
            }
        }
    }

    auto Util::calculateNearest2Pow( int input ) -> int
    {
        if( input <= 32 )
        {
            return 32;
        }
        if( input <= 64 )
        {
            return 64;
        }
        if( input <= 128 )
        {
            return 128;
        }
        if( input <= 256 )
        {
            return 256;
        }
        if( input <= 512 )
        {
            return 512;
        }
        if( input <= 1024 )
        {
            return 1024;
        }
        if( input <= 2048 )
        {
            return 2048;
        }
        return input;
    }

    auto Util::addMenuItem( SmartPtr<ui::IUIMenu> menu, hash_type itemid, const String &text,
                            const String &help, ui::IUIMenuItem::Type type ) -> SmartPtr<ui::IUIMenuItem>
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto ui = applicationManager->getUI();

        auto menuItem = ui->addElementByType<ui::IUIMenuItem>();
        WP_ASSERT( menuItem );

        menuItem->setElementId( itemid );
        menuItem->setText( text );
        menuItem->setHelp( help );
        menuItem->setMenuItemType( type );

        menu->addMenuItem( menuItem );

        return menuItem;
    }

    auto Util::addMenuSeparator( SmartPtr<ui::IUIMenu> menu ) -> SmartPtr<ui::IUIMenuItem>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto ui = applicationManager->getUI();
        WP_ASSERT( ui );

        auto menuItem = ui->addElementByType<ui::IUIMenuItem>();
        WP_ASSERT( menuItem );

        auto type = ui::IUIMenuItem::Type::Separator;
        menuItem->setMenuItemType( type );

        menu->addMenuItem( menuItem );

        return menuItem;
    }

    auto Util::setText( SmartPtr<ui::IUITreeNode> node, const String &text ) -> SmartPtr<ui::IUIElement>
    {
        if( node )
        {
            auto applicationManager = core::IApplicationManager::instance();

            if( auto ui = applicationManager->getUI() )
            {
                if( auto textElement = ui->addElementByType<ui::IUIText>() )
                {
                    textElement->setText( text );
                    node->addChild( textElement );

                    return textElement;
                }
            }
        }

        return nullptr;
    }

    auto Util::setImage( SmartPtr<ui::IUITreeNode> node, const String &imagePath )
        -> SmartPtr<ui::IUIElement>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto ui = applicationManager->getUI();
        WP_ASSERT( ui );

        auto imageElement = ui->addElementByType<ui::IUIImage>();
        WP_ASSERT( imageElement );

        //textElement->setText( text );
        node->setNodeData( imageElement );

        return imageElement;
    }

    auto Util::getFirstChild( SmartPtr<ui::IUIElement> element ) -> SmartPtr<ui::IUIElement>
    {
        if( element )
        {
            auto children = element->getChildren();
            if( !children.empty() )
            {
                return children.front();
            }
        }

        return nullptr;
    }

    auto Util::getText( SmartPtr<ui::IUITreeNode> node ) -> String
    {
        auto children = node->getChildren();
        for( auto &child : children )
        {
            if( child->isDerived<ui::IUIText>() )
            {
                auto text = workphone::static_pointer_cast<ui::IUIText>( child );
                return text->getText();
            }
        }

        return {};
    }

    ParameterType Util::getParameterTypeFromStringPtr( const StringPtr typeStr )
    {
        if( StringUtil::isNullOrEmpty( typeStr ) )
        {
            return ParameterType::PARAM_TYPE_STR;
        }

        return {};
    }

    ParameterType Util::getParameterTypeFromString( const String &typeStr )
    {
        if( typeStr.empty() )
        {
            return ParameterType::PARAM_TYPE_STR;
        }

        if( StringUtil::isEqual( typeStr, Properties::colourStr ) )
        {
            return ParameterType::PARAM_TYPE_COLOUR;
        }
        else if( StringUtil::isEqual( typeStr, Properties::colourfStr ) )
        {
            return ParameterType::PARAM_TYPE_COLOUR;
        }
        else if( StringUtil::isEqual( typeStr, Util::colouriStr ) )
        {
            return ParameterType::PARAM_TYPE_COLOURI;
        }
        else if( StringUtil::isEqual( typeStr, Properties::stringArrayStr ) )
        {
            return ParameterType::PARAM_TYPE_ARRAY;
        }
        else if( StringUtil::isEqual( typeStr, Properties::aabbStr ) )
        {
            return ParameterType::PARAM_TYPE_AABB3;
        }
        else if( StringUtil::isEqual( typeStr, Properties::aabbdStr ) )
        {
            return ParameterType::PARAM_TYPE_AABB3D;
        }

        if( typeStr == Properties::stringTypeStr || typeStr == defaultType )
        {
            return ParameterType::PARAM_TYPE_STR;
        }
        else if( typeStr == Properties::intTypeStr )
        {
            return ParameterType::PARAM_TYPE_S32;
        }
        else if( typeStr == Properties::u32Str )
        {
            return ParameterType::PARAM_TYPE_U32;
        }

        if( typeStr == Properties::unsignedLongStr )
        {
            return ParameterType::PARAM_TYPE_U32;
        }
        else if( typeStr == Properties::unsignedLongLongStr )
        {
            return ParameterType::PARAM_TYPE_S64;
        }
        else if( typeStr == Properties::floatStr )
        {
            return ParameterType::PARAM_TYPE_F32;
        }

        else if( typeStr == Properties::doubleTypeStr )
        {
            return ParameterType::PARAM_TYPE_F64;
        }
        else if( typeStr == Properties::boolTypeStr )
        {
            return ParameterType::PARAM_TYPE_BOOL;
        }
        else if( typeStr == Properties::noneStr )
        {
            return ParameterType::PARAM_TYPE_NULL;
        }
        else if( typeStr == "u8" )
        {
            return ParameterType::PARAM_TYPE_U8;
        }
        else if( typeStr == "u16" )
        {
            return ParameterType::PARAM_TYPE_U16;
        }
        else if( typeStr == "u32" )
        {
            return ParameterType::PARAM_TYPE_U32;
        }
        else if( typeStr == "s8" )
        {
            return ParameterType::PARAM_TYPE_S8;
        }
        else if( typeStr == "s16" )
        {
            return ParameterType::PARAM_TYPE_S16;
        }
        else if( typeStr == "s32" )
        {
            return ParameterType::PARAM_TYPE_S32;
        }
        else if( typeStr == "f32" )
        {
            return ParameterType::PARAM_TYPE_F32;
        }
        else if( typeStr == "s64" )
        {
            return ParameterType::PARAM_TYPE_S64;
        }
        else if( typeStr == "f64" )
        {
            return ParameterType::PARAM_TYPE_F64;
        }
        else if( typeStr == "char_ptr" )
        {
            return ParameterType::PARAM_TYPE_CHAR_PTR;
        }
        else if( typeStr == "ptr" )
        {
            return ParameterType::PARAM_TYPE_PTR;
        }
        else if( StringUtil::isEqual( typeStr, Util::vector2iStr ) )
        {
            return ParameterType::PARAM_TYPE_VEC2I;
        }
        else if( StringUtil::isEqual( typeStr, Util::vector2fStr ) )
        {
            return ParameterType::PARAM_TYPE_VEC2F;
        }
        else if( StringUtil::isEqual( typeStr, Util::vector2dStr ) )
        {
            return ParameterType::PARAM_TYPE_VEC2D;
        }
        else if( StringUtil::isEqual( typeStr, Util::vector3iStr ) )
        {
            return ParameterType::PARAM_TYPE_VEC3I;
        }
        else if( StringUtil::isEqual( typeStr, Util::vector3fStr ) )
        {
            return ParameterType::PARAM_TYPE_VEC3F;
        }
        else if( StringUtil::isEqual( typeStr, Util::vector3dStr ) )
        {
            return ParameterType::PARAM_TYPE_VEC3D;
        }
        else if( typeStr == "quatf" )
        {
            return ParameterType::PARAM_TYPE_QUATF;
        }
        else if( typeStr == "quatd" )
        {
            return ParameterType::PARAM_TYPE_QUATD;
        }
        else if( typeStr == "transform3" )
        {
            return ParameterType::PARAM_TYPE_TRANSFORM3;
        }
        else if( typeStr == "colour" )
        {
            return ParameterType::PARAM_TYPE_COLOUR;
        }

        if( typeStr == Properties::resourceStr )
        {
            return ParameterType::PARAM_TYPE_RESOURCE;
        }
        else if( typeStr == Properties::materialStr )
        {
            return ParameterType::PARAM_TYPE_OBJECT;
        }
        else if( typeStr == Properties::textureStr )
        {
            return ParameterType::PARAM_TYPE_TEXTURE;
        }
        else if( typeStr == Properties::componentStr )
        {
            return ParameterType::PARAM_TYPE_COMPONENT;
        }
        else if( typeStr == Properties::buttonTypeStr )
        {
            return ParameterType::PARAM_TYPE_BUTTON;
        }
        else if( typeStr == Properties::enumTypeStr )
        {
            return ParameterType::PARAM_TYPE_ENUM;
        }

        return (ParameterType)0;
    }

    String Util::getStringFromParameterType( ParameterType type )
    {
        switch( type )
        {
        case ParameterType::PARAM_TYPE_NULL:
            return "null";
        case ParameterType::PARAM_TYPE_BOOL:
            return "bool";
        case ParameterType::PARAM_TYPE_U8:
            return "u8";
        case ParameterType::PARAM_TYPE_U16:
            return "u16";
        case ParameterType::PARAM_TYPE_U32:
            return "u32";
        case ParameterType::PARAM_TYPE_S8:
            return "s8";
        case ParameterType::PARAM_TYPE_S16:
            return "s16";
        case ParameterType::PARAM_TYPE_S32:
            return "s32";
        case ParameterType::PARAM_TYPE_F32:
            return "f32";
        case ParameterType::PARAM_TYPE_S64:
            return "s64";
        case ParameterType::PARAM_TYPE_F64:
            return "f64";
        case ParameterType::PARAM_TYPE_CHAR_PTR:
            return "char_ptr";
        case ParameterType::PARAM_TYPE_PTR:
            return "ptr";
        case ParameterType::PARAM_TYPE_BUTTON:
            return "button";
        case ParameterType::PARAM_TYPE_OBJECT:
            return "object";
        case ParameterType::PARAM_TYPE_COMPONENT:
            return "component";
        case ParameterType::PARAM_TYPE_TEXTURE:
            return "texture";
        case ParameterType::PARAM_TYPE_RESOURCE:
            return "resource";
        case ParameterType::PARAM_TYPE_STR:
            return "string";
        case ParameterType::PARAM_TYPE_VEC2I:
            return "vector2i";
        case ParameterType::PARAM_TYPE_VEC2F:
            return "vector2f";
        case ParameterType::PARAM_TYPE_VEC2D:
            return "vector2d";
        case ParameterType::PARAM_TYPE_VEC3I:
            return "vector3i";
        case ParameterType::PARAM_TYPE_VEC3F:
            return "vector3f";
        case ParameterType::PARAM_TYPE_VEC3D:
            return "vector3d";
        case ParameterType::PARAM_TYPE_QUATF:
            return "quatf";
        case ParameterType::PARAM_TYPE_QUATD:
            return "quatd";
        case ParameterType::PARAM_TYPE_TRANSFORM3:
            return "transform3";
        case ParameterType::PARAM_TYPE_COLOUR:
            return "colour";
        case ParameterType::PARAM_TYPE_COLOURI:
            return "colouri";
        case ParameterType::PARAM_TYPE_ARRAY:
            return "array";
        case ParameterType::PARAM_TYPE_ENUM:
            return enumTypeStr;
        }

        return {};
    }

    const StringPtr Util::getStringFromParameterTypePtr( ParameterType type )
    {
        switch( type )
        {
        case ParameterType::PARAM_TYPE_NULL:
            return (const StringPtr)nullStr.c_str();
        case ParameterType::PARAM_TYPE_ENUM:
            return (const StringPtr)enumTypeStr.c_str();
        };

        return nullptr;
    }

}  // namespace workphone
