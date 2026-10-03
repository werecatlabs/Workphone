#include <WPWxWidgets/WPWxWidgetsPCH.hpp>
#include "WPWxWidgets/PropGridCommon.hpp"
#include <wx/propgrid/propgrid.hpp>

// -----------------------------------------------------------------------
// wxVectorProperty
// -----------------------------------------------------------------------

// See propgridsample.h for wxVector3f class

WX_PG_IMPLEMENT_VARIANT_DATA_DUMMY_EQ( wxVector3f )

WX_PG_IMPLEMENT_PROPERTY_CLASS( wxVectorProperty, wxPGProperty, wxVector3f, const wxVector3f &,
                                TextCtrl )

wxVectorProperty::wxVectorProperty( const wxString &label, const wxString &name,
                                    const wxVector3f &value ) :
    wxPGProperty( label, name )
{
    SetValue( WXVARIANT( value ) );
    AddPrivateChild( new wxFloatProperty( wxT( "X" ), wxPG_LABEL, value.x ) );
    AddPrivateChild( new wxFloatProperty( wxT( "Y" ), wxPG_LABEL, value.y ) );
    AddPrivateChild( new wxFloatProperty( wxT( "Z" ), wxPG_LABEL, value.z ) );
}

wxVectorProperty::~wxVectorProperty()
{
}

void wxVectorProperty::RefreshChildren()
{
    if( !GetChildCount() )
        return;

    const wxVector3f &vector = wxVector3fRefFromVariant( m_value );
    Item( 0 )->SetValue( vector.x );
    Item( 1 )->SetValue( vector.y );
    Item( 2 )->SetValue( vector.z );
}

wxVariant wxVectorProperty::ChildChanged( wxVariant &thisValue, int childIndex,
                                          wxVariant &childValue ) const
{
    wxVector3f vector;
    vector << thisValue;
    switch( childIndex )
    {
    case 0:
        vector.x = childValue.GetDouble();
        break;
    case 1:
        vector.y = childValue.GetDouble();
        break;
    case 2:
        vector.z = childValue.GetDouble();
        break;
    }
    wxVariant newVariant;
    newVariant << vector;
    return newVariant;
}
