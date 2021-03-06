// START A3HEADER
//
// This source file is part of Atlantis GUI
// Copyright (C) 2003-2004 Ben Lloyd
//
// To be used with the Atlantis PBM game program.
// Copyright (C) 1995-2004 Geoff Dunbar
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program, in the file license.txt. If not, write
// to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.
//
// See the Atlantis Project web page for details:
// http://www.prankster.com/project
//
// END A3HEADER
//
// Mapping functionality based on Atlantis Little Helper, by Max
// Shariy


#include "map.h"
#include "gui.h"

#include "../alist.h"
#include "../aregion.h"
#include "config.h"

#include <math.h>
#include "wx/toolbar.h"
#include "wx/image.h"
#include "bitmaps/shaft.xpm"
#include "bitmaps/gate.xpm"
#include "bitmaps/object.xpm"
#include "bitmaps/city.xpm"
#include "bitmaps/arrowup.xpm"
#include "bitmaps/arrowdown.xpm"
#include "bitmaps/zoom_in.xpm"
#include "bitmaps/zoom_out.xpm"
#include "bitmaps/map.xpm"
#include "bitmaps/coord.xpm"
#include "bitmaps/label.xpm"
#include "bitmaps/outline.xpm"

#include "gamedata.h"

const double cos30 = sqrt( (double) 3 ) / 2;
const double tan30 = 1/sqrt( (double) 3 );

// ---------------------------------------------------------------------------
// event tables
// ---------------------------------------------------------------------------

BEGIN_EVENT_TABLE( MapFrame, wxWindow )
	EVT_MENU( MAP_PLANE_DOWN, MapFrame::OnPlaneDown )
	EVT_MENU( MAP_PLANE_UP, MapFrame::OnPlaneUp )
	EVT_MENU( MAP_ZOOM_IN, MapFrame::OnZoomIn )
	EVT_MENU( MAP_ZOOM_OUT, MapFrame::OnZoomOut )
	EVT_MENU( MAP_SHOW_CITIES, MapFrame::OnShowCities )
	EVT_MENU( MAP_SHOW_OBJECTS, MapFrame::OnShowObjects )
	EVT_MENU( MAP_SHOW_GATES, MapFrame::OnShowGates )
	EVT_MENU( MAP_SHOW_SHAFTS, MapFrame::OnShowShafts )
	EVT_MENU( MAP_SHOW_COORDS, MapFrame::OnShowCoords )
	EVT_MENU( MAP_SHOW_NAMES, MapFrame::OnShowNames )
	EVT_MENU( MAP_SHOW_OUTLINES, MapFrame::OnShowOutlines )
	EVT_COMBOBOX( MAP_LEVEL, MapFrame::OnLevelSelect )
	EVT_SIZE( MapFrame::OnResize )
END_EVENT_TABLE()

BEGIN_EVENT_TABLE( MapCanvas, wxWindow )
	EVT_MOUSE_EVENTS( MapCanvas::OnMouse )
	EVT_PAINT( MapCanvas::OnPaint )
	EVT_SCROLLWIN( MapCanvas::OnScroll )
	EVT_SIZE( MapCanvas::OnResize )
END_EVENT_TABLE()

// ---------------------------------------------------------------------------
// MapCanvas
// ---------------------------------------------------------------------------

/**
 * Default constructor
 */
MapCanvas::MapCanvas( wxWindow *parent )
		  : wxWindow( parent, -1, wxDefaultPosition, wxDefaultSize,
					  wxSUNKEN_BORDER |
					  wxVSCROLL | wxHSCROLL | wxNO_FULL_REPAINT_ON_RESIZE )
{
	mapParent = ( MapFrame * ) parent;
	hexSize = MapConfig.lastZoom;
	SetVirtualSizeHints( 100, 100 );

	selectedElems = new AElemArray();
	tempSelection = new AElemArray();
	curSelection = -1;

	SetBackgroundColour( *wxBLACK );

	InitPlane( MapConfig.lastPlane );
	DrawMap( true );

	yscroll = 0;
	xscroll = 0;

	lastX = 0 - MapConfig.lastX;
	lastY = 0 - MapConfig.lastY;

}

/**
 * Destructor
 */
MapCanvas::~MapCanvas()
{
	MapConfig.lastX = GetScrollPos( wxHORIZONTAL );
	MapConfig.lastY = GetScrollPos( wxVERTICAL );
	MapConfig.lastZoom = hexSize;
	MapConfig.lastPlane = planeNum;

//	delete sizerMap;
//	delete sizerTool;
	delete selectedElems;
}

/**
 * Change the size of the map hexes
 */
void MapCanvas::SetHexSize( int width )
{
	hexSize = width;
	hexHalfSize   = hexSize / 2;
	hexHalfHeight = ( int )( hexSize * cos30 + 0.5 );	 
	hexHeight	  = hexHalfHeight * 2;
}

/**
 * Prepare DC and clear map if necessary, then draw map
 */
void MapCanvas::DrawMap( bool clearFirst )
{
	wxClientDC dc( this );

	if( clearFirst )
		dc.Clear();
	DrawMap( &dc );
}

/**
 * Draw map onto DC
 */
void MapCanvas::DrawMap( wxDC * pDC )
{
	if( !app->m_game )
		return;

	forlist( &app->m_game->regions ) {
		ARegion * r = ( ARegion * ) elem;
		if( r->zloc == planeNum )
			DrawHex( r, pDC, -1, 0 );
	}

	DrawOverlay( pDC );
	// temporary fix
	DrawCoords( pDC );
	DrawNames( pDC );
}

/**
 * Draw map overlay: borders, co-ordinates and town names
 */
void MapCanvas::DrawOverlay( wxDC * pDC )
{
	DrawBorders( pDC );
	DrawCoords( pDC );
	DrawNames( pDC );
}

/**
 * Draw town names on DC
 */
void MapCanvas::DrawNames( wxDC * pDC )
{
	if( !MapConfig.showNames ) return;
	forlist( &app->m_game->regions ) {
		ARegion * r = ( ARegion * ) elem;
		if( r->zloc != planeNum ) continue;
		if( !r->town ) continue;
		int x0, y0;
		GetHexCenter( r->xloc, r->yloc, x0, y0 );

		DrawName( r->town, x0, y0, pDC );
	}
}

/**
 * Draw map co-ordinates onto DC
 */
void MapCanvas::DrawCoords( wxDC * pDC )
{
	if( !MapConfig.showCoords ) return;

	pDC->SetTextBackground( *wxBLACK );
	int fontSize = hexSize / 4 + 3;
	wxFont mapFont( fontSize, wxROMAN, wxNORMAL, wxNORMAL );
	pDC->SetFont( mapFont );

	{
		//draw 0
		wxString num;
		num << 0;
		int xPt = hexSize - (int) ((double)fontSize / 2.7 * (double)num.Len()) - xscroll;
		int yPt = -yscroll;
		DrawString(num, xPt, yPt, pDC );
	}

	for(int x = 2 ; x < m_plane->x-1; x += 2 ) {
		wxString num;
		num << x;
		int xPt = x * hexSize * 3 / 2 + hexSize - (int) ((double)fontSize / 2.7 * (double)num.Len()) - xscroll;
		int yPt = -yscroll;
		pDC->SetTextForeground( *wxBLACK );
		pDC->DrawText(num, xPt+1, yPt+1);
		pDC->SetTextForeground( *wxWHITE );
		pDC->DrawText(num, xPt, yPt);
		DrawString(num, xPt, yPt, pDC );
	}

	for( int y = 2; y < m_plane->y-1; y += 2 ) {
		wxString num;
		num << y;
		int yPt = y/2 * hexHeight - yscroll; 
		int xPt = hexSize - (int) ((double)fontSize / 2.7 * (double)num.Len()) - xscroll;
		pDC->SetTextForeground( *wxBLACK );
		pDC->DrawText(num, xPt+1, yPt+1);
		pDC->SetTextForeground( *wxWHITE );
		pDC->DrawText(num, xPt, yPt);
		DrawString(num, xPt, yPt, pDC );
	}
	
}

/**
 * Draw hex borders onto DC
 */
void MapCanvas::DrawBorders( wxDC * pDC )
{
	int x0, y0;

	//hilighted borders
	wxPen * pen = wxWHITE_PEN;
	if( curSelection != SELECT_REGION ) pen = wxRED_PEN;

	for( int i = 0; i < (int) selectedElems->GetCount() ; i++ ) {
		ARegion * r = ( ARegion * ) selectedElems->Item( i );
		if( r->zloc == planeNum ) {
			GetHexCenter( r->xloc, r->yloc, x0, y0 );
			DrawAHex( x0, y0, pen, wxTRANSPARENT_BRUSH, pDC,
				      r->neighbors );
		}
	}
}

/**
 * Draw a complete single hex
 * Redraw neighbours as well (needed if hex previously had a label that crossed into
 *  neighbouring hexes, and now it doesn't)
 */
void MapCanvas::DrawHex( ARegion * pRegion, wxDC * pDC, int highlight, int drawName )
{
	wxBrush terrBrush;
	wxPen borderPen;

	// Get Hex Center
	if( !pRegion ) return;
	if( pRegion->zloc != planeNum ) return;

	int x0, y0;
	GetHexCenter( pRegion->xloc, pRegion->yloc, x0, y0 );

	int height,width;
	GetClientSize( &width,&height );

	if( x0 < 0 - hexSize || x0 > width + hexSize ||
		y0 < 0 - hexHeight || y0 > height + hexHeight )
	{
		return;
	}

	borderPen.SetColour( wxColour( _T( "BLACK" ) ) );

	if( highlight == -1 )
	{
		bool drawSides = false;
		// Edge hexes - terrain only
		int x1,y1;
		ARegion * edgeHex;

		ARegion **exits;
		if( pRegion->xloc == 0 ) {
			//draw SW and NW neighbours
			edgeHex = pRegion->neighbors[4];// SW
			x1 = x0 - hexSize *3/2;
			y1 = y0 + hexHalfHeight;
			// kludge: don't draw nexus neighbours
			if( edgeHex && edgeHex->zloc == pRegion->zloc && drawSides ) {
				SetTerrainBrush( edgeHex->type, terrBrush, 0 );
				exits = edgeHex->neighbors;
			} else {
				terrBrush.SetColour( wxColour( _T( "BLACK" ) ) );
				exits = NULL;
			}
			DrawAHex( x1, y1, &borderPen, &terrBrush, pDC, exits );
	
			edgeHex = pRegion->neighbors[5];// NW
			y1 = y0 - hexHalfHeight;
			if( edgeHex && edgeHex->zloc == pRegion->zloc && drawSides ) {
				SetTerrainBrush( edgeHex->type, terrBrush, 0 );
				exits = edgeHex->neighbors;
			} else {
				terrBrush.SetColour( wxColour( _T( "BLACK" ) ) );
				exits = NULL;
			}
			DrawAHex( x1, y1, &borderPen, &terrBrush, pDC, exits );
		}
		if( pRegion->xloc == m_plane->x-1 && 
			m_plane->x-1 * hexSize *3/2 < width ) {
			//draw NE and SE neighbours
			edgeHex = pRegion->neighbors[1];// NE

			x1 = x0 + hexSize*3/2;
			y1 = y0 - hexHalfHeight;
			if( edgeHex && edgeHex->zloc == pRegion->zloc && drawSides ) {
				SetTerrainBrush( edgeHex->type, terrBrush, 0 );
				exits = edgeHex->neighbors;
			} else {
				terrBrush.SetColour( wxColour( _T( "BLACK" ) ) );
				exits = NULL;
			}
			DrawAHex( x1, y1, &borderPen, &terrBrush, pDC, exits );
			edgeHex = pRegion->neighbors[2];// SE
			y1 = y0 + hexHalfHeight;
			if( edgeHex && edgeHex->zloc == pRegion->zloc && drawSides ) {
				SetTerrainBrush( edgeHex->type, terrBrush, 0 );
				exits = edgeHex->neighbors;
			} else {
				terrBrush.SetColour( wxColour( _T( "BLACK" ) ) );
				exits = NULL;
			}
			DrawAHex( x1, y1, &borderPen, &terrBrush, pDC, exits );
		}
		if( pRegion->yloc == 1 ) {
			//draw N neighbour
			edgeHex = pRegion->neighbors[0];// N
			x1 = x0;
			y1 = y0 - hexHeight;
			if( edgeHex && edgeHex->zloc == pRegion->zloc && drawSides ) {
				SetTerrainBrush( edgeHex->type, terrBrush, 0 );
				exits = edgeHex->neighbors;
			} else {
				terrBrush.SetColour( wxColour( _T( "BLACK" ) ) );
				exits = NULL;
			}
			DrawAHex( x1, y1, &borderPen, &terrBrush, pDC, exits );
		}
		if( pRegion->yloc == m_plane->y-2 &&
			m_plane->y-1 * hexHeight < height ) {
			//draw S neighbour
			edgeHex = pRegion->neighbors[3];// S
			x1 = x0;
			y1 = y0 + hexHeight;
			if( edgeHex && edgeHex->zloc == pRegion->zloc && drawSides ) {
				SetTerrainBrush( edgeHex->type, terrBrush, 0 );
				exits = edgeHex->neighbors;
			} else {
				terrBrush.SetColour( wxColour( _T( "BLACK" ) ) );
				exits = NULL;
			}
			DrawAHex( x1, y1, &borderPen, &terrBrush, pDC, exits );
		}
	}

	if( highlight == -1 ) {
		if( selectedElems->Index( pRegion ) == wxNOT_FOUND )
			highlight = 0;
		else if( curSelection == SELECT_REGION )
			highlight = 1;
		else
			highlight = 2;
	}

	SetTerrainBrush( pRegion->type, terrBrush, highlight );

	wxImage * image = GetTerrainTile( pRegion->type, highlight );

	DrawAHex( x0, y0, &borderPen, &terrBrush, pDC, pRegion->neighbors, image );

	// Symbols
	int SymLeft   = x0 - hexHalfSize;
	int SymBottom = y0 + hexHalfHeight - 2;

	//	City	
	if( pRegion->town )
		DrawTown( pRegion->town->TownType(), x0, y0, pDC );
	   
	//	Gate
	if( pRegion->gate )
	{
		DrawGate( SymLeft, SymBottom, pDC );
		GetNextIconPos( x0, y0, SymLeft, SymBottom );
	}

	// Structures
	bool drawShaft = false;
	int shaftLevel = 0;
	bool drawBoat = false;
	bool drawOther = false;

	forlist( &pRegion->objects ) {
		Object * o = ( Object * ) elem;
		if( o->IsRoad() ) {
			DrawRoad( x0, y0, pDC, o );
		} else if( o->type == O_SHAFT ) {
			drawShaft = true;
			shaftLevel = pRegion->zloc - app->m_game->regions.GetRegion( o->inner )->zloc;
		} else if( o->IsBoat() ) {
			drawBoat = true;
		} else if( o->type != O_DUMMY ) {
			drawOther = true;
		}
	}

	if( drawShaft ) {
		DrawShaft( SymLeft, SymBottom, pDC, shaftLevel );			 
		GetNextIconPos( x0, y0, SymLeft, SymBottom );
	}
	if( drawBoat ) {
		DrawBoat( SymLeft, SymBottom, pDC );			 
		GetNextIconPos( x0, y0, SymLeft, SymBottom );
	}
	if( drawOther ) {
		DrawOther( SymLeft, SymBottom, pDC );			 
		GetNextIconPos( x0, y0, SymLeft, SymBottom );
	}

	if( drawName && pRegion->town ) {
		DrawName( pRegion->town, x0, y0, pDC );
	}

}

/**
 * Draw a single hex: border and terrain bitmap/colour only
 */
void MapCanvas::DrawAHex( int x, int y, wxPen *pen, wxBrush *brush, wxDC *pDC,
						  ARegion * exits[], wxImage * image )
{
	if( x < 0 - hexHalfSize ) return;
	if( y < 0 - hexHalfHeight ) return;
	
	if( x > GetSize().x + hexHalfSize ) return;
	if( y > GetSize().y + hexHalfHeight ) return;

	if( image && GuiConfig.showFunk ) {
		wxBitmap bmp( image->Scale( hexSize * 2, hexHalfHeight * 2 ) );
		wxMemoryDC mDC;
		mDC.SelectObject( bmp );
		pDC->Blit( x - hexSize, y - hexHalfHeight, bmp.GetWidth(), bmp.GetHeight(), &mDC, 0, 0, wxCOPY, true );	
	}

	wxPoint point[7]	=  
	{ 
		wxPoint( x - hexHalfSize, y - hexHalfHeight ),	 //1
		wxPoint( x + hexHalfSize, y - hexHalfHeight ),	 //2
		wxPoint( x + hexSize	, y 				  ),   //3
		wxPoint( x + hexHalfSize, y + hexHalfHeight ),	 //4
		wxPoint( x - hexHalfSize, y + hexHalfHeight ),	 //5
		wxPoint( x - hexSize	, y 				  ),	//6
		wxPoint( x - hexHalfSize, y - hexHalfHeight ),	 //0
	};

	if( MapConfig.showOutlines )
		pDC->SetPen( *pen );
	else
		pDC->SetPen( *wxTRANSPARENT_PEN );

	if( image && GuiConfig.showFunk ) {
		pDC->SetBrush( *wxTRANSPARENT_BRUSH );
	} else {
		pDC->SetBrush( *brush );
	}

	pDC->DrawPolygon( 6, point, 0, 0, wxWINDING_RULE );

	wxPen pen2( pen->GetColour(), 3, wxSOLID );
	pDC->SetPen( pen2 );
	if( exits ) {
		for( int i = 0; i < NDIRS; i++ )	{
			if( exits[i] == 0 ) {
				pDC->DrawLine( point[i].x, point[i].y, point[i+1].x, point[i+1].y);
			}
		}
	}
	pDC->SetPen( *pen );
}

/**
 * Process a scroll event
 * Redraw map at new co-ordinates
 */
void MapCanvas::OnScroll( wxScrollWinEvent& event )
{
	int curX, curY, oldX, oldY;

	oldX = curX = GetScrollPos( wxHORIZONTAL );
	oldY = curY = GetScrollPos( wxVERTICAL );

	
	if( event.m_eventType == wxEVT_SCROLLWIN_LINEUP )
	{
		if( event.GetOrientation() == wxHORIZONTAL ) 
			curX -= 1;
		else
			curY -= 1;
	}
	else if( event.m_eventType == wxEVT_SCROLLWIN_LINEDOWN )
	{
		if( event.GetOrientation() == wxHORIZONTAL ) 
			curX += 1;
		else
			curY += 1;
	}
	else if( event.m_eventType == wxEVT_SCROLLWIN_PAGEUP )
	{
		if( event.GetOrientation() == wxHORIZONTAL )
			curX -= GetScrollThumb( wxHORIZONTAL );
		else
			curY -= GetScrollThumb( wxVERTICAL );
	}
	else if( event.m_eventType == wxEVT_SCROLLWIN_PAGEDOWN )
	{
		if( event.GetOrientation() == wxHORIZONTAL )
			curX += GetScrollThumb( wxHORIZONTAL );
		else
			curY += GetScrollThumb( wxVERTICAL );
	}
	else if( event.m_eventType == wxEVT_SCROLLWIN_THUMBTRACK )
	{
		if( event.GetOrientation() == wxHORIZONTAL )
			curX = event.GetPosition();
		else
			curY = event.GetPosition();
	}

	int maxX = GetScrollRange( wxHORIZONTAL ) - GetScrollThumb( wxHORIZONTAL );
	int maxY = GetScrollRange( wxVERTICAL ) - GetScrollThumb( wxVERTICAL );

	if( maxX < 0 ) maxX = 0;
	if( maxY < 0 ) maxY = 0;

	if( curX < 0 ) curX = 0;
	if( curX > maxX ) curX = maxX;
	if( curY < 0 ) curY = 0;
	if( curY > maxY ) curY = maxY;

	if( oldY == curY && oldX == curX ) return;

	SetScrollPos( wxHORIZONTAL, curX );
	SetScrollPos( wxVERTICAL, curY );

	xscroll = curX * hexSize;
	yscroll = curY * hexHeight;

	DrawMap();
}

/**
 * Process a mouse event
 * Update status bar to reflect co-ordinates under mouse
 * Select a hex if LMB clicked
 * Select all similar contiguous hexes if LMB double-clicked
 */
void MapCanvas::OnMouse( wxMouseEvent& event )
{
	long xpos,ypos;
	int hexX, hexY;

	static ARegion * lastRegion = 0;
	static bool controlWasDown = false;

	xpos = event.GetX();
	ypos = event.GetY();
	GetHexCoord( hexX, hexY, xpos, ypos );

	ARegion * pRegion;

	if( hexX < 0 || hexX > m_plane->x-1 ||
		hexY < 0 || hexY > m_plane->y-1 ) pRegion = NULL;
	else
		pRegion = app->m_game->regions.GetRegion( hexX, hexY, planeNum );

	app->UpdateStatusBar( pRegion );
//	app->UpdateStatusBarDebug( xpos, ypos, hexX, hexY, GetScrollPos( wxHORIZONTAL ), GetScrollPos( wxVERTICAL ) , hexSize, hexHeight );
//	app->UpdateStatusBarDebug( xpos, ypos, hexX, hexY, GetScrollPos( wxHORIZONTAL ), GetScrollPos( wxVERTICAL ) , rx, ry );

	if( !pRegion ) return;

	if( event.LeftDClick() ) {
		forlist( &app->m_game->regions ) {
			((ARegion *) elem)->checked = 0;
		}
		SpreadSelection( pRegion, *((int *) mapParent->comboSelect->GetClientData( mapParent->comboSelect->GetSelection() ) ));
		if( event.ShiftDown() ) {
			forlist( &app->m_game->regions ) {
				ARegion * r = ( ARegion * ) elem;
				if( app->IsSelected( r ) ) r->checked = 1;
			}
		}
		AElemArray newSel;
		{
			forlist( &app->m_game->regions ) {
				ARegion * r = (ARegion * ) elem;
				if( r->checked ) newSel.Add( r );
				// small speed fix, may not work if levels are added later...
				if( r->zloc > planeNum ) break;
			}
		}
		app->UpdateSelection( &newSel, SELECT_REGION );
	} else if( event.LeftDown() ) {
		if( event.ShiftDown() /*|| event.ControlDown() */ ) {
			app->Select( pRegion, true );
		} else {
			if( selectedElems->GetCount() == 1 && lastRegion == pRegion ) {
				// don't want to reselect the region again for no reason
				return;
			}
			app->Select( pRegion, false );
		}
		lastRegion = pRegion;
		app->UpdateSelection();
	} else {
		if( event.ControlDown() ) {
			if( pRegion == lastRegion ) return;
			lastRegion = pRegion;
			app->Select( pRegion, true );
			app->UpdateSelection();
		}	
	}
}

/**
 * Recursively check through a region's neighbours to see if they
 * should be included in the new selection. If so, the region is
 * 'checked'.
 */
void MapCanvas::SpreadSelection( ARegion * pRegion, int spreadBy )
{
	pRegion->checked = 1;
	for( int i = 0; i < NDIRS; i++ ) {
		ARegion * r = pRegion->neighbors[i];
		if( !r ) continue;
		if( r->checked ) continue;
		bool spread = false;
		switch( spreadBy ) {
			case SPREAD_TERRAIN:
				if( r->type == pRegion->type ) spread = true;
				break;
			case SPREAD_PROVINCE:
				if( *r->name == *pRegion->name ) spread = true;
				break;
			case SPREAD_RACE:
				if( r->race == pRegion->race ) spread = true;
				break;
		}
		if( spread ) SpreadSelection( r, spreadBy );
	}
}
	
/**
 * Get the hex at the center of the screen (Not working yet)
 */
ARegion * MapCanvas::GetCenterHex()
{
	int hexX, hexY;

	int x = GetClientSize().x / 2;
	int y = GetClientSize().y / 2;
	GetHexCoord( hexX, hexY, x, y );

	ARegion * pRegion;

	if( hexX < 0 || hexX > m_plane->x-1 ||
		hexY < 0 || hexY > m_plane->y-1 ) pRegion = NULL;
	else
		pRegion = app->m_game->regions.GetRegion( hexX, hexY, planeNum );

	return pRegion;
}

/**
 * Adjust map to make a particular hex at the center
 */
void MapCanvas::SetCenterHex( ARegion * pRegion )
{
	int maxX, maxY;
	GetMaxScroll( maxX, maxY );

	double xp = (double) pRegion->xloc / (double) hexesX;
	double yp = (double) pRegion->yloc / (double) hexesY;

	int x = (int) ((double) maxX * xp );
	int y = (int) ((double) maxY * yp );

	if( x < 0 ) x = 0;
	if( x > maxX ) x = maxX;
	if( y < 0 ) y = 0;
	if( y > maxY ) y = maxY;

	SetScrollPos( wxHORIZONTAL, x );
	SetScrollPos( wxVERTICAL, y );

	xscroll = x * hexSize;
	yscroll = y * hexHeight;

}

/**
 * Find the map co-ordinates of the center of the specified hex co-ordinates
 */
void MapCanvas::GetHexCenter( int NoX, int NoY, int & WinX, int & WinY )
{
	// center of ( 0,0 ) hex has 0,0 Atla coordinates

	long AtlaX, AtlaY;

	AtlaX = NoX * hexSize * 3 / 2 + hexSize;
	AtlaY = NoY * hexHalfHeight + hexHalfHeight;

	AtlaToWin( WinX, WinY, AtlaX, AtlaY );

}

void MapCanvas::GetHexCoord( int & NoX, int & NoY, int WinX, int WinY )
{
	// center of ( 0,0 ) hex has 0,0 Atla coordinates

	long AtlaX, AtlaY;
	int  ApprX, ApprY; //approximite hex no
	int  x, y, x0, y0;
	int  xx, yy;
	xx = -1;  yy = -1;

	WinToAtla( WinX, WinY, AtlaX, AtlaY );

	ApprX = ( AtlaX - hexSize ) / ( hexSize * 3 / 2 ); // hexSize must be even!
	ApprY = ( AtlaY - hexHalfHeight ) / hexHalfHeight;

	int rise, run;
	double maxdist = hexSize *200;
	double dist;
	for( x = ApprX-1; x <= ApprX+1; x++ )
	{
		for( y = ApprY-2; y <= ApprY+2; y++ )
		{
			if( ValidHexNo( x,y ) )
			{
				GetHexCenter( x,y,x0,y0 );

				// find distance from this hex center to the map point
				rise = ( x0 - WinX ) ;
				run = ( y0 - WinY );
				dist = sqrt( (double) (rise*rise + run*run ) );


				if( dist<maxdist ) {
					xx = x0; yy = y0; NoX = x; NoY = y; maxdist = dist;
				}

			}
		}
	}
}

int MapCanvas::ValidHexNo( int NoX, int NoY )
{	
	// sum of coordinates must be even
	int x = NoX + NoY;

	return x == ( ( x>>1 )<<1 );
}

inline void MapCanvas::WinToAtla( int	 WinX,  int    WinY,
								  long & AtlaX, long & AtlaY )
{
	AtlaX = WinX + xscroll;
	AtlaY = WinY + yscroll;
}

inline void MapCanvas::AtlaToWin( int &  WinX,  int &  WinY,
								  long   AtlaX, long   AtlaY )
{
	WinX = AtlaX - xscroll;
	WinY = AtlaY - yscroll;
}

void MapCanvas::OnPaint( wxPaintEvent &WXUNUSED( event ) )
{
	wxPaintDC dc( this );
	DrawMap( &dc );
}


void MapCanvas::DrawName( TownInfo * town, int x0, int y0, wxDC * pDC )
{
	if( !MapConfig.showNames ) return;

	int width,height;
	GetClientSize( &width, &height );

	if( x0 < 0 - hexSize || x0 > width + hexSize ||
		y0 < 0 - hexHeight || y0 > height + hexHeight )
	{
		return;
	}

	pDC->SetTextBackground( *wxBLACK);
	int fontSize = hexSize / 4 + 3;
	wxFont mapFont( fontSize, wxROMAN, wxNORMAL, wxNORMAL );
	pDC->SetFont( mapFont );

	wxString name( town->name->Str() );
	y0 -= hexHalfHeight * 4 / 3;
	x0 -= ((double)fontSize / 3.2 * ((double)name.Len() ));
	DrawString( name, x0, y0, pDC );
}

void MapCanvas::DrawString( wxString & name, int x0, int y0, wxDC * pDC )
{
	pDC->SetTextForeground( *wxBLACK );
	pDC->DrawText(name, x0+1, y0+1);
	pDC->SetTextForeground( *wxWHITE );
	pDC->DrawText(name, x0, y0);
}

void MapCanvas::DrawTown( int townSize, int x0, int y0, wxDC * pDC )
{
	if( !MapConfig.showCities ) return;
	int arcSize = hexHalfHeight / 3 + townSize - 1;
	int col = 180 - 90 * townSize;

	wxColour townColour( 255, col, col );

	pDC->SetPen  ( *wxBLACK_PEN );
	pDC->SetBrush( wxBrush( townColour, wxSOLID ) );

	pDC->DrawArc( x0 - arcSize, y0, x0 - arcSize, y0, x0, y0 );
}

void MapCanvas::DrawRoad( int x0, int y0, wxDC * pDC, Object * obj )
{
	if( !MapConfig.showObjects ) return;

	wxPen roadPen;
	int x2,y2;

	roadPen.SetColour( 112, 128, 144 );
	if( obj->incomplete ) {
		wxDash dash[2];
		dash[0] = 1; dash[1] = 2;
		roadPen.SetDashes( 2, dash );
	}
		
	pDC->SetPen( roadPen );

	switch ( obj->type ) {
	case O_ROADN:
		pDC->DrawLine( x0, y0, x0, y0 - hexHalfHeight );
		break;
	case O_ROADS:
		pDC->DrawLine( x0, y0, x0, y0 + hexHalfHeight );
		break;
	case O_ROADSE:
		x2 = x0 + hexHalfSize * 3 / 2; 
		y2 = y0 + hexHalfHeight / 2;
		pDC->DrawLine( x0, y0, x2, y2 );
		break;
	case O_ROADSW:
		x2 = x0 - hexHalfSize * 3 / 2; 
		y2 = y0 + hexHalfHeight / 2;
		pDC->DrawLine( x0, y0, x2, y2 );
		break;
	case O_ROADNE:
		x2 = x0 + hexHalfSize * 3 / 2; 
		y2 = y0 - hexHalfHeight / 2;
		pDC->DrawLine( x0, y0, x2, y2 );
		break;
	case O_ROADNW:
		x2 = x0 - hexHalfSize * 3 / 2; 
		y2 = y0 - hexHalfHeight / 2;
		pDC->DrawLine( x0, y0, x2, y2 );
		break;
	}
}

void MapCanvas::GetNextIconPos( int x0, int y0, int & x, int & y )
{
	x += ( ICON_SIZE+1 );

	// assume we never get out of the lower part of a hex
		
	if( x + ICON_SIZE > x0 + hexHalfSize +
		( long )( tan30*( y0 + hexHalfHeight - y ) ) ) 
	{
		y -= ( ICON_SIZE+1 );
		x = x0 - hexHalfSize - ( long )( tan30*( y0 + hexHalfHeight - y ) );
	}
}

void MapCanvas::DrawGate( int x0, int y0, wxDC * pDC )
{
	if( !MapConfig.showGates ) return;

	pDC->SetPen ( *wxBLACK_PEN );
	pDC->SetBrush( *wxBLACK_BRUSH );
	pDC->DrawRectangle( x0, y0 - ICON_SIZE, ICON_SIZE, ICON_SIZE );

	pDC->SetBrush( *wxCYAN_BRUSH );
	pDC->DrawArc( x0 + ICON_SIZE / 2, y0, 
				 x0 + ICON_SIZE / 2, y0, 
				 x0 + ICON_SIZE / 2, y0 - ICON_SIZE / 2 );
}

void MapCanvas::DrawShaft( int x0, int y0, wxDC * pDC, int shaftLevel )
{
	if( !MapConfig.showShafts ) return;

	pDC->SetPen  ( *wxWHITE_PEN );
	pDC->SetBrush( *wxWHITE_BRUSH );

	if( shaftLevel > 0 ) {
		// draw upwards shaft arrow
		wxPoint tri[3] = 
		{
			wxPoint( x0, y0 - 1 ),
			wxPoint( x0 + ICON_SIZE, y0 - 1 ),
			wxPoint( x0 + ICON_SIZE / 2, y0 - ICON_SIZE + 1)
		};
		pDC->DrawPolygon( 3, tri, 0, 0, wxWINDING_RULE );
	} else if( shaftLevel < 0 ) {
		// draw downwards shaft arrow
		wxPoint tri[3] = 
		{
			wxPoint( x0, y0 - ICON_SIZE + 1 ),
			wxPoint( x0 + ICON_SIZE, y0 - ICON_SIZE + 1 ),
			wxPoint( x0 + ICON_SIZE / 2, y0 - 1)
		};
		pDC->DrawPolygon( 3, tri, 0, 0, wxWINDING_RULE );
	}//	} else if( shaftLevel == 0 ) {
		// draw normal shaft icon
		pDC->SetPen  ( *wxBLACK_PEN );
		pDC->DrawLine( x0 + 1, y0 - ICON_SIZE, x0 + 1, y0 );
		pDC->DrawLine( x0 + ICON_SIZE - 1, y0 - ICON_SIZE, x0 + ICON_SIZE - 1, y0 );

		pDC->DrawLine( x0 + 1, y0 - 2, x0 + ICON_SIZE - 1, y0 - 2 );
		pDC->DrawLine( x0 + 1, y0 - ICON_SIZE + 2, x0 + ICON_SIZE - 1, y0 - ICON_SIZE + 2 );
//	}
}

void MapCanvas::DrawBoat( int x0, int y0, wxDC * pDC )
{
	if( !MapConfig.showObjects ) return;

	wxPoint tri[3] = 
	{
		wxPoint( x0, y0 - ICON_SIZE ),
		wxPoint( x0 + ICON_SIZE / 2, y0 - 1 ),
		wxPoint( x0 + ICON_SIZE, y0 - ICON_SIZE )
	};
	pDC->SetPen  ( *wxBLACK_PEN );
	pDC->SetBrush( *wxWHITE_BRUSH );
	pDC->DrawPolygon( 3, tri, 0, 0, wxWINDING_RULE );
}

void MapCanvas::DrawOther( int x0, int y0, wxDC * pDC )
{
	if( !MapConfig.showObjects ) return;
	pDC->SetPen  ( *wxWHITE_PEN );
	pDC->SetBrush( *wxTRANSPARENT_BRUSH );
	pDC->DrawRectangle( x0, y0 - ICON_SIZE, ICON_SIZE, ICON_SIZE );
}

void MapCanvas::AdjustScrollBars()
{
	int width, height;
	GetClientSize( &width,&height );

	SetScrollbar( wxVERTICAL, GetScrollPos( wxHORIZONTAL ), height / hexHeight,
		          GetScrollRange( wxVERTICAL ) );
	SetScrollbar( wxHORIZONTAL, GetScrollPos( wxHORIZONTAL ),
		          width / ( hexSize ), GetScrollRange( wxHORIZONTAL ) );
}

void MapCanvas::OnResize( wxSizeEvent& event )
{
	AdjustScrollBars();
}


void MapCanvas::InitPlane( int p, ARegion * center )
{
	planeNum = p;

	m_plane = app->m_game->regions.pRegionArrays[p];

	hexesX = m_plane->x;
	hexesY = m_plane->y;

	SetHexSize( hexSize );

	SetScrollbar( wxVERTICAL, 0, 5, (hexesY + 1) / 2 );
	SetScrollbar( wxHORIZONTAL, 0, 5, (hexesX + 1) * 3/2 );

	AdjustScrollBars();
	if( center ) {
		SetCenterHex( center );
	} else if( lastX <= 0 && lastY <= 0 ) {
		SetScrollPos( wxHORIZONTAL, 0 - lastX );
		SetScrollPos( wxVERTICAL, 0 - lastY );

		xscroll = 0 - lastX * hexSize;
		yscroll = 0 - lastY * hexHeight;
	} else {
		int maxX, maxY;
		GetMaxScroll( maxX, maxY );
		int x = (int) ((double) maxX * lastX);
		int y = (int) ((double) maxY * lastY);

		SetScrollPos( wxHORIZONTAL, x );
		SetScrollPos( wxVERTICAL, y );

		xscroll = x * hexSize;
		yscroll = y * hexHeight;
	}

	AString title = AString( "Level " ) + p;
	if( m_plane->strName )
		title += AString(" : ") + *m_plane->strName;
	SetTitle( title.Str() );
}

int MapCanvas::MoveUpLevel()
{
	if( planeNum > 0 ) 
		InitPlane( planeNum - 1 ); //< If succesful, will change planeNum to new value
	return planeNum;
}

int MapCanvas::MoveDownLevel()
{
	if( planeNum < app->m_game->regions.numLevels -1 )
		InitPlane( planeNum + 1 ); //< If succesful, will change planeNum to new value
	return planeNum;
}

void MapFrame::OnPlaneDown( wxCommandEvent & event )
{
	int plane = canvas->MoveDownLevel();

	toolbar->EnableTool( MAP_PLANE_UP, true );
	if( plane >= app->m_game->regions.numLevels -1 )
		toolbar->EnableTool( MAP_PLANE_DOWN, false );
	comboLevel->SetSelection( plane );
	canvas->DrawMap( true );
}

void MapFrame::OnLevelSelect( wxCommandEvent & event )
{
	int plane = event.GetSelection();
	comboLevel->SetSelection( plane );
	canvas->InitPlane( plane );
	canvas->DrawMap( true );
}

void MapCanvas::GetMaxScroll( int & maxX, int & maxY )
{
	maxX = GetScrollRange( wxHORIZONTAL ) - GetScrollThumb( wxHORIZONTAL );
	maxY = GetScrollRange( wxVERTICAL ) - GetScrollThumb( wxVERTICAL );
	if( maxX < 0 ) maxX = 0;
	if( maxY < 0 ) maxY = 0;
}

int MapCanvas::ZoomIn()
{
	if( hexSize >= 30 ) hexSize +=4;
	if( hexSize >= 20 ) hexSize +=4;
	hexSize += 4;

	int maxX, maxY;
	GetMaxScroll( maxX, maxY );

	lastX = (double) GetScrollPos( wxHORIZONTAL ) / (double) maxX;
	lastY = (double) GetScrollPos( wxVERTICAL ) / (double) maxY;

	InitPlane( planeNum );
	return hexSize;
}

void MapFrame::OnZoomIn( wxCommandEvent & event )
{
	if( canvas ) {
		int hexSize = canvas->ZoomIn();
		toolbar->EnableTool( MAP_ZOOM_OUT, true );
		if( hexSize >= 40 ) toolbar->EnableTool( MAP_ZOOM_IN, false );		
		canvas->DrawMap( true );
	}
}

void MapFrame::OnZoomOut( wxCommandEvent & event )
{
	if( canvas ) {
		int hexSize = canvas->ZoomOut();
		toolbar->EnableTool( MAP_ZOOM_IN, true );
		if( hexSize <= 4 ) toolbar->EnableTool( MAP_ZOOM_OUT, false );
		canvas->DrawMap( true );
	}
}

int MapCanvas::ZoomOut()
{

	hexSize -= 4;
	if( hexSize >= 20 ) hexSize -=4;
	if( hexSize >= 30 ) hexSize -=4;

	int maxX, maxY;
	GetMaxScroll( maxX, maxY );

	lastX = (double) GetScrollPos( wxHORIZONTAL ) / (double) maxX;
	lastY = (double) GetScrollPos( wxVERTICAL ) / (double) maxY;

	InitPlane( planeNum );
	return hexSize;
}

void MapFrame::OnShowCities( wxCommandEvent & event )
{
	MapConfig.showCities = toolbar->GetToolState( MAP_SHOW_CITIES );
	canvas->DrawMap();
}

void MapFrame::OnShowObjects( wxCommandEvent & event )
{
	MapConfig.showObjects = toolbar->GetToolState( MAP_SHOW_OBJECTS );
	canvas->DrawMap();
}

void MapFrame::OnShowGates( wxCommandEvent & event )
{
	MapConfig.showGates = toolbar->GetToolState( MAP_SHOW_GATES );
	canvas->DrawMap();
}

void MapFrame::OnShowShafts( wxCommandEvent & event )
{
	MapConfig.showShafts = toolbar->GetToolState( MAP_SHOW_SHAFTS );
	canvas->DrawMap();
}

void MapFrame::OnShowNames( wxCommandEvent & event )
{
	MapConfig.showNames = toolbar->GetToolState( MAP_SHOW_NAMES );
	canvas->DrawMap();
}

void MapFrame::OnShowOutlines( wxCommandEvent & event )
{
	MapConfig.showOutlines = toolbar->GetToolState( MAP_SHOW_OUTLINES );
	canvas->DrawMap();
}

void MapFrame::OnShowCoords( wxCommandEvent & event )
{
	MapConfig.showCoords = toolbar->GetToolState( MAP_SHOW_COORDS );
	canvas->DrawMap();
}

void MapCanvas::UpdateSelection()
{
	int i;

	// If we're selecting something other than regions at the moment
	// we want to show the regions that those things (eg units) are in.
	// Create temporary array of selections here first
	tempSelection->Clear();
	if( app->curSelection == SELECT_OBJECT ) {
		for( i = 0; i < (int) app->selectedElems->GetCount(); i++ ) {
			Object * o = (Object * ) app->selectedElems->Item( i );
			if( tempSelection->Index( o->region ) == wxNOT_FOUND )
				tempSelection->Add( o->region );
		}
	} else if( app->curSelection == SELECT_UNIT ) {
		for( i = 0; i < (int) app->selectedElems->GetCount(); i++ ) {
			Unit * u = (Unit * ) app->selectedElems->Item( i );
			if( tempSelection->Index( u->object->region ) == wxNOT_FOUND )
				tempSelection->Add( u->object->region );
		}
	} else if( app->curSelection == SELECT_MARKET ) {
		for( i = 0; i < (int) app->selectedElems->GetCount(); i++ ) {
			Market * m = (Market * ) app->selectedElems->Item( i );
			ARegion * r = app->m_game->regions.GetRegion( m->region );
			if( tempSelection->Index( r ) == wxNOT_FOUND )
				tempSelection->Add( r );
		}
	} else if( app->curSelection == SELECT_PRODUCTION ) {
		for( i = 0; i < (int) app->selectedElems->GetCount(); i++ ) {
			Production * p = (Production * ) app->selectedElems->Item( i );
			ARegion * r = app->m_game->regions.GetRegion( p->region );
			if( tempSelection->Index( r ) == wxNOT_FOUND )
				tempSelection->Add( r );
		}
	}

	// First make sure we've got the right level
	int level = planeNum;
	if( app->curSelection == SELECT_LEVEL ) {
		if( app->selectedLevel ) 
			level = app->selectedLevel->GetRegion(0,0)->zloc;
	} else if( app->curSelection == SELECT_GAME ||
		       app->curSelection == SELECT_FACTION ) {
		level = planeNum;
	} else if( app->curSelection == SELECT_REGION ) {
		if( app->selectedElems->GetCount() > 0 )
			level = ((ARegion *) app->selectedElems->Item(0))->zloc;
	} else {
		if( tempSelection->GetCount() > 0 )
			level = ((ARegion *) tempSelection->Item(0))->zloc;
	}

	if( level != planeNum ) {
		InitPlane( level );
		DrawMap( true );
	}

	// Prepare canvas for drawing
	wxClientDC dc( this );

	// Change selections
	AElemArray * array = 0;
	if( app->curSelection == SELECT_LEVEL ||
		app->curSelection == SELECT_GAME ||
		app->curSelection == SELECT_FACTION )
	{
		// Unhighlight everything now
		for( i = 0; i < (int) selectedElems->GetCount(); i++ ) {
			ARegion * r = ( ARegion * ) selectedElems->Item( i );
			DrawHex( r, &dc, 0 );
		}
	} else if( app->curSelection == SELECT_REGION ) {
		array = app->selectedElems;
	} else {
		array = tempSelection;
	}


	if( array ) {
		// Force reselection if selection colours are different
		int force = 0;
		if( app->curSelection != curSelection && 
			( app->curSelection == SELECT_REGION || curSelection == SELECT_REGION ) )
			force = 1;

		// Unhighlight any items that are not in the new selection
		for( i = 0; i < (int) selectedElems->GetCount(); i++ ) {
			ARegion * r = ( ARegion * ) selectedElems->Item( i );
			if( array->Index( r ) == wxNOT_FOUND || force )
				DrawHex( r, &dc, 0 );
		}
	
		int highlight;
		if( app->curSelection == SELECT_REGION )
			highlight = 1;
		else
			highlight = 2;

		// Now Highlight any items that are not in the current selection
		for( i = 0; i < (int) array->GetCount(); i++ ) {
			ARegion * r = ( ARegion * ) array->Item( i );
			if( selectedElems->Index( r ) == wxNOT_FOUND || force )
				DrawHex( r, &dc, highlight );
		}
	}


	// Synch selection values
	curSelection = app->curSelection;
	selectedElems->Clear();
	if( array == tempSelection ) {
		delete selectedElems;
		selectedElems = tempSelection;
		tempSelection = new AElemArray();
	} else if( array ) {
		for( i = 0; i < (int) array->GetCount(); i++ ) {
			AListElem * elem = app->selectedElems->Item( i );
			selectedElems->Add( elem );
		}
	}

	DrawOverlay( &dc );

}

// ---------------------------------------------------------------------------
// MapFrame
// ---------------------------------------------------------------------------
MapFrame::MapFrame( wxWindow *parent )
         : wxWindow( parent, -1, wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_FRAME_STYLE, "Map" )
{
	comboLevel = 0;
	comboSelect = 0;
	SetBackgroundColour( app->guiColourLt );

	toolbar = new wxToolBar( this, -1, wxDefaultPosition, wxDefaultSize,
							 wxNO_BORDER | wxTB_HORIZONTAL | wxTB_FLAT );
	InitToolBar( toolbar, MapConfig.lastPlane );

	canvas = new MapCanvas( this );

}

MapFrame::~MapFrame()
{
	MapConfig.spreadBy = *( (int *)comboSelect->GetClientData( comboSelect->GetSelection() ) );

	delete comboLevel;
	for( int i = 0; i < comboSelect->GetCount(); i++ )
		if( comboSelect->GetClientData( i ) )
			delete ( int * ) comboSelect->GetClientData( i );
	delete comboSelect;
}

void MapFrame::UpdateSelection()
{
	if( canvas )
		canvas->UpdateSelection();
}

void MapFrame::OnResize( wxSizeEvent& event )
{
	int w,h;
	GetClientSize( &w, &h );
	if( w>0 && h>0 ) {
		int offset = toolbar->GetSize().y;
		toolbar->SetSize( 0, 0, w, offset );
		canvas->SetSize( 0, offset+1, w, h-offset );
	}
}

/**
 * Initialise Toolbar
 */
void MapFrame::InitToolBar( wxToolBar * toolBar, int level )
{
	wxBitmap* bitmaps[11];

	bitmaps[0] = new wxBitmap( arrowup_xpm );
	bitmaps[1] = new wxBitmap( arrowdown_xpm );
	bitmaps[2] = new wxBitmap( zoom_in_xpm );
	bitmaps[3] = new wxBitmap( zoom_out_xpm );
	bitmaps[4] = new wxBitmap( city_xpm );
	bitmaps[5] = new wxBitmap( object_xpm );
	bitmaps[6] = new wxBitmap( gate_xpm );
	bitmaps[7] = new wxBitmap( shaft_xpm );
	bitmaps[8] = new wxBitmap( label_xpm );
	bitmaps[9] = new wxBitmap( coord_xpm );
	bitmaps[10] = new wxBitmap( outline_xpm );

	comboLevel = new wxComboBox( toolBar, MAP_LEVEL, "", wxDefaultPosition, wxDefaultSize,
	                             0, NULL, wxCB_READONLY );

	int i;
	for( i = 0; i < app->m_game->regions.numLevels; i++ )  {
		AString * name = app->m_game->regions.GetRegionArray( i )->strName;
		AString temp = AString( "[" ) + i + "] ";
		if( !name ) {
			temp += "Unnamed";
		} else {
			temp += *name;
		}
		comboLevel->Append( temp.Str(), app->m_game->regions.GetRegionArray( i ) );
	}
	comboLevel->SetSelection( level );

	comboSelect = new wxComboBox( toolBar, MAP_SELECT, "", wxDefaultPosition, wxDefaultSize,
	                             0, NULL, wxCB_READONLY );
	comboSelect->Append( "Terrain", new int( SPREAD_TERRAIN ) );
	comboSelect->Append( "Province", new int( SPREAD_PROVINCE ) );
	comboSelect->Append( "Race", new int( SPREAD_RACE ) );

	toolBar->SetToolSeparation( 80 );

	toolBar->AddTool( MAP_PLANE_UP, "", *( bitmaps[0] ), "Up one plane" );
	toolBar->AddTool( MAP_PLANE_DOWN, "", *( bitmaps[1] ), "Down one plane" );
	toolBar->AddControl( comboLevel );
	toolBar->AddSeparator();
	toolBar->AddTool( MAP_ZOOM_IN, "", *( bitmaps[2] ), "Zoom in" );
	toolBar->AddTool( MAP_ZOOM_OUT, "", *( bitmaps[3] ), "Zoom out" );
	toolBar->AddSeparator();
	toolBar->AddCheckTool( MAP_SHOW_CITIES, "", *( bitmaps[4] ), *( bitmaps[4] ), "Show cities" );
	toolBar->AddCheckTool( MAP_SHOW_OBJECTS, "", *( bitmaps[5] ), *( bitmaps[5] ), "Show objects" );
	toolBar->AddCheckTool( MAP_SHOW_GATES, "", *( bitmaps[6] ), *( bitmaps[6] ), "Show gates" );
	toolBar->AddCheckTool( MAP_SHOW_SHAFTS, "", *( bitmaps[7] ), *( bitmaps[7] ), "Show shafts" );
	toolBar->AddCheckTool( MAP_SHOW_NAMES, "", *( bitmaps[8] ), *( bitmaps[8] ), "Show town names" );
	toolBar->AddCheckTool( MAP_SHOW_COORDS, "", *( bitmaps[9] ), *( bitmaps[9] ), "Show map co-ordinates" );
	toolBar->AddCheckTool( MAP_SHOW_OUTLINES, "", *( bitmaps[10] ), *( bitmaps[10] ), "Show hex outlines" );
	toolBar->AddSeparator();
	wxStaticText * text = new wxStaticText( toolBar, -1, " Select: " );
	toolBar->AddControl( text );
	toolBar->AddControl( comboSelect );
	comboSelect->SetSelection( MapConfig.spreadBy );

	toolBar->ToggleTool( MAP_SHOW_CITIES, MapConfig.showCities );
	toolBar->ToggleTool( MAP_SHOW_OBJECTS, MapConfig.showObjects );
	toolBar->ToggleTool( MAP_SHOW_GATES, MapConfig.showGates );
	toolBar->ToggleTool( MAP_SHOW_SHAFTS, MapConfig.showShafts );
	toolBar->ToggleTool( MAP_SHOW_NAMES, MapConfig.showNames );
	toolBar->ToggleTool( MAP_SHOW_COORDS, MapConfig.showCoords );
	toolBar->ToggleTool( MAP_SHOW_OUTLINES, MapConfig.showOutlines );

	text->SetBackgroundColour( app->guiColourLt );
	toolBar->SetBackgroundColour( app->guiColourLt );

	for( i = 0; i < 11; i++ )
		delete bitmaps[i];

	if( level < app->m_game->regions.numLevels -1 )
		toolBar->EnableTool( MAP_PLANE_DOWN, true );
	else
		toolBar->EnableTool( MAP_PLANE_DOWN, false );

	if( level > 0 )
		toolBar->EnableTool( MAP_PLANE_UP, true );
	else
		toolBar->EnableTool( MAP_PLANE_UP, false );

	toolBar->Realize();
}

void MapFrame::OnPlaneUp( wxCommandEvent & event )
{
	int plane = canvas->MoveUpLevel();

	toolbar->EnableTool( MAP_PLANE_DOWN, true );
	if( plane <= 0 ) toolbar->EnableTool( MAP_PLANE_UP, false );
	comboLevel->SetSelection( plane );

	canvas->DrawMap( true );
}

