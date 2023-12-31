//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CLASSMENU_H
#define CLASSMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/HTML.h>
#include <UtlVector.h>
#include <vgui/ILocalize.h>
#include <vgui/KeyCode.h>
#include <cl_dll/iviewport.h>

#include "mouseoverpanelbutton.h"

namespace vgui
{
	class TextEntry;
}

//-----------------------------------------------------------------------------
// Purpose: Draws the class menu
//-----------------------------------------------------------------------------
class CClassMenu : public vgui::Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE( CClassMenu, vgui::Frame );

public:
	CClassMenu(IViewPort *pViewPort);
	virtual ~CClassMenu();

	virtual const char *GetName( void ) { return PANEL_CLASS; }
	virtual void SetData(KeyValues *data);
	virtual void Reset() {};
	virtual void Update() {};
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }

protected:

	virtual vgui::Panel *CreateControlByName(const char *controlName);
	virtual MouseOverPanelButton* CreateNewMouseOverPanelButton(vgui::Panel *panel);

	//vgui2 overrides
	virtual void OnKeyCodePressed(vgui::KeyCode code);

	// helper functions
	void SetLabelText(const char *textEntryName, const char *text);
	void SetVisibleButton(const char *textEntryName, bool state);

	// command callbacks
	void OnCommand( const char *command );

	IViewPort	*m_pViewPort;
	int			m_iScoreBoardKey;
	int			m_iTeam;
	vgui::Panel *m_pPanel;

	MouseOverPanelButton *m_pFirstButton;
};


#endif // CLASSMENU_H
