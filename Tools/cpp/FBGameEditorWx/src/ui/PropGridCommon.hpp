#ifndef _PropGridCommon_H
#define _PropGridCommon_H



#include <GameEditorPrerequisites.hpp>
#include <wx/wx.hpp>
#include <wx/propgrid/propgrid.hpp>



// -----------------------------------------------------------------------
class wxVector3f
{
public:
	wxVector3f()
	{
		x = y = z = 0.0;
	}

	wxVector3f( double _x, double _y, double _z )
	{
		x = _x; y = _y; z = _z;
	}

	double x, y, z;
};



inline bool operator == (const wxVector3f& a, const wxVector3f& b)
{
	return (a.x == b.x && a.y == b.y && a.z == b.z);
}



WX_PG_DECLARE_VARIANT_DATA(wxVector3f)



class wxVectorProperty : public wxPGProperty
{
	WX_PG_DECLARE_PROPERTY_CLASS(wxVectorProperty)
public:

	wxVectorProperty( const wxString& label = wxPG_LABEL,
		const wxString& name = wxPG_LABEL,
		const wxVector3f& value = wxVector3f() );
	virtual ~wxVectorProperty();

	virtual wxVariant ChildChanged( wxVariant& thisValue,
		int childIndex,
		wxVariant& childValue ) const;
	virtual void RefreshChildren();

protected:
};


// -----------------------------------------------------------------------

//WX_PG_DECLARE_ARRAYSTRING_PROPERTY_WITH_VALIDATOR_WITH_DECL(wxDirsProperty, class wxEMPTY_PARAMETER_VALUE)







#endif