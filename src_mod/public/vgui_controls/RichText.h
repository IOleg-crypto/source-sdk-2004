//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef RICHTEXT_H
#define RICHTEXT_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <UtlVector.h>

namespace vgui
{

class ClickPanel;

//-----------------------------------------------------------------------------
// Purpose: Non-editable display of a rich text control
//-----------------------------------------------------------------------------
class RichText : public Panel
{
	DECLARE_CLASS_SIMPLE( RichText, Panel );

public:
	RichText(Panel *parent, const char *panelName);
	~RichText();

	// text manipulation
	void SetText(const char *text);
	void SetText(const wchar_t *text);
	void GetText(int offset, wchar_t *buf, int bufLenInBytes);

	// configuration
	void SetFont(HFont font);

	// inserts characters at the end of the stream
	void InsertChar(wchar_t ch);
	void InsertString(const char *text);
	void InsertString(const wchar_t *wszText);
	
	// selection
	void SelectNone();
	void SelectAllText();
	void SelectNoText();
	MESSAGE_FUNC( CutSelected, "DoCutSelected" );
	MESSAGE_FUNC( CopySelected, "DoCopySelected" );

	// cursor movement
	void GotoTextStart();	// go to start of text buffer
	void GotoTextEnd();	// go to end of text buffer

	// configuration
	// sets visibility of scrollbar
	void SetVerticalScrollbar(bool state);
	// sets limit of number of characters insertable into field; set to -1 to remove maximum
	// only works with if rich-edit is NOT enabled
	void SetMaximumCharCount(int maxChars);

	// rich edit commands
	void InsertColorChange(Color col);
	// IndentChange doesn't take effect until the next newline character
	void InsertIndentChange(int pixelsIndent);
	// clickable text
	// notification that text was clicked is through "TextClicked" message
	void InsertClickableTextStart();
	void InsertClickableTextEnd();
	// inserts a string that needs to be scanned for urls/mailto commands to be made clickable
	void InsertPossibleURLString(const char *text, Color URLTextColor, Color normalTextColor);

	// sets the height of the window so all text is visible.
	// used by tooltips
	void SetToFullHeight();
	int GetNumLines();

	/* CUSTOM MESSAGE HANDLING
		"SetText"
			input:	"text"	- text is set to be this string
	*/

	/* MESSAGE SENDING (to action signal targets)
		"TextChanged"	- sent when the text is edited by the user
			
		
		"TextClicked"	- sent when clickable text has been clicked on
			"text"	- the text that was clicked on
	*/

	virtual bool RequestInfo(KeyValues *outputData);
	/* INFO HANDLING
		"GetText"
			returns:
				"text" - text contained in the text box
	*/
	virtual void SetFgColor( Color color );

protected:
	virtual void OnThink();
	virtual void PerformLayout();  // layout the text in the window
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void PaintBackground();

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void GetSettings( KeyValues *outResourceData );
	virtual const char *GetDescription( void );
	MESSAGE_FUNC_WCHARPTR( OnSetText, "SetText", text );
	MESSAGE_FUNC( OnSliderMoved, "ScrollBarSliderMoved" ); // respond to scroll bar events
	virtual void OnKillFocus();
	virtual void OnMouseWheeled(int delta);	// respond to mouse wheel events
	virtual void OnKeyCodeTyped(KeyCode code);	//respond to keyboard events
	
	MESSAGE_FUNC_INT( OnClickPanel, "ClickPanel", index);

	virtual void OnCursorMoved(int x, int y);  // respond to moving the cursor with mouse button down
	virtual void OnMousePressed(MouseCode code); // respond to mouse down events
	virtual void OnMouseDoublePressed(MouseCode code);
	virtual void OnMouseReleased(MouseCode code);	// respond to mouse up events

	virtual void OnMouseFocusTicked(); // do while window has mouse focus
	virtual void OnCursorEntered();	 // handle cursor entering window
	virtual void OnCursorExited();	 // handle cursor exiting window

	virtual void OnMouseCaptureLost(); 
	virtual void OnSizeChanged(int newWide, int newTall);
	virtual void OnSetFocus();

	// clickable url handling
	int ParseTextStringForUrls(const char *text, int startPos, char *resultBuffer, int resultBufferSize, bool &clickable);
	virtual void OnTextClicked(const wchar_t *text);

private:
	void GotoWordRight();	// move cursor to start of next word
	void GotoWordLeft();	// move cursor to start of prev word

	void TruncateTextStream();
	bool GetSelectedRange(int& cx0,int& cx1);
	void CursorToPixelSpace(int cursorPos, int &cx, int &cy);
	int PixelToCursorSpace(int cx, int cy);
	void AddAnotherLine(int &cx, int &cy);
	void RecalculateDefaultState(int startIndex);

	void LayoutVerticalScrollBarSlider();
	void OpenEditMenu();
	void FinishingURL(int x, int y);
	// Returns the character index the drawing should Start at
	int GetStartDrawIndex(int &lineBreakIndexIndex);
	int GetCursorLine();
	void MoveScrollBar(int delta);
	int GetClickableTextIndexStart(int startIndex); 
	void CreateEditMenu(); // create copy/cut/paste menu

	// linebreak stream functions
	void InvalidateLineBreakStream();
	void RecalculateLineBreaks();

	// format stream - describes changes in formatting for the text stream
	struct TFormatStream
	{
		// render state
		Color color;
		int pixelsIndent;
		bool textClickable;

		// position in TextStream that these changes take effect
		int textStreamIndex;
	};

	// data
	CUtlVector<wchar_t>   m_TextStream;		// the text in the text window is stored in this buffer
	CUtlVector<int>	   m_LineBreaks;		// an array that holds the index in the buffer to wrap lines at
	CUtlVector<TFormatStream> m_FormatStream;	// list of format changes

	bool m_bRecalcLineBreaks;

	int	_recalculateBreaksIndex;			// tells next linebreakindex index to Start recalculating line breaks	
	bool			   _invalidateVerticalScrollbarSlider;
	int                _cursorPos;			// the position in the text buffer of the blinking cursor
	bool			   _mouseSelection;		// whether we are highlighting text or not (selecting text)
	bool			   _mouseDragSelection;	// tells weather mouse is outside window and button is down so we select text
	int                _select[2];			// select[1] is the offset in the text to where the cursor is currently
											// select[0] is the offset to where the cursor was dragged to. or -1 if no drag.
	int				   _pixelsIndent;
	int				   _maxCharCount;		// max number of chars that can be in the text buffer
	HFont              _font;				// font of chars in the text buffer
	Color			   _selectionColor;
	Color			   _selectionTextColor;	// color of the highlighted text
	bool			   _currentTextClickable;
	Dar<ClickPanel *>  _clickableTextPanels;
	int				   _clickableTextIndex;
	Color				_defaultTextColor;
	int					_drawOffsetX;
	int					_drawOffsetY;


	// sub-controls
	ScrollBar			*_vertScrollBar;	// the scroll bar used in the window
	Menu				*m_pEditMenu;		// cut/copy/paste popup

	char				*m_pszInitialText;	// initial text

	// saved state
	bool _recalcSavedRenderState;
	
	struct TRenderState
	{
		// rendering positions
		int x, y;
		
		// basic state
		Color textColor;
		int pixelsIndent;
		bool textClickable;

		// index into our current position in the formatting stream
		int formatStreamIndex;
	};
	TRenderState m_CachedRenderState;	// cached render state for the beginning of painting

	// updates a render state based on the formatting and color streams
	// returns true if any state changed
	bool UpdateRenderState(int textStreamPos, TRenderState &renderState);

	void GenerateRenderStateForTextStreamIndex(int textStreamIndex, TRenderState &renderState);
	int FindFormatStreamIndexForTextStreamPos(int textStreamIndex);

	// draws a single character with the current render state
	int DrawChar(wchar_t ch, TRenderState &renderState, HFont font, int index);
};

} // namespace vgui


#endif // RICHTEXT_H
