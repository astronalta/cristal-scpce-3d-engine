/////////////////////////////////////////////////////////////////////////////
// Copyright:   (c) Aleksandras Gluchovas
// Licence:     wxWindows license
//
// Ported to Crystal Space by Norman Kr�mer <norman@users.sourceforge.net>
/////////////////////////////////////////////////////////////////////////////

#include "cssysdef.h"
#include "csws/csbaglay.h"
#include "csutil/hashmap.h"

csGridBagConstraint::csGridBagConstraint (csComponent *comp)

  : csLayoutConstraint (comp),
    gridx( RELATIVE ),
    gridy( RELATIVE ),
    gridwidth( 1 ),
    gridheight( 1 ),
    weightx( 0 ),
    weighty( 0 ),
    anchor ( CENTER ),
    fill   ( NONE ),
    insets ( 0,0,0,0 ),
    ipadx  ( 0 ),
    ipady  ( 0 )
{}

csGridBagConstraint::csGridBagConstraint (csComponent *comp, int _gridx,
					  int _gridy,
					  int _gridwidth,
					  int _gridheight,
					  double _weightx,
					  double _weighty,
					  int _anchor,
					  int _fill,
					  csInsets _insets,
					  int _ipadx,
					  int _ipady )
  : csLayoutConstraint (comp),
    gridx( _gridx ),
    gridy( _gridy ),
    gridwidth ( _gridwidth  ),
    gridheight( _gridheight ),
    weightx( _weightx ),
    weighty( _weighty ),
    anchor ( _anchor ),
    fill   ( _fill ),
    insets ( _insets ),
    ipadx  ( _ipadx ),
    ipady  ( _ipady )
{}

csGridBagConstraint::csGridBagConstraint (const csGridBagConstraint &c) : csLayoutConstraint (c.comp)
{
  gridx = c.gridx;
  gridy = c.gridy;
  gridwidth = c.gridwidth;
  gridheight = c.gridheight;
  weightx = c.weightx;
  weighty = c.weighty;
  anchor = c.anchor;
  fill = c.fill;
  insets.left = c.insets.left;
  insets.top = c.insets.top;
  insets.bottom = c.insets.bottom;
  insets.right = c.insets.right;
  ipadx = c.ipadx;
  ipady = c.ipady;
 
}

csLayoutConstraint *csGridBagConstraint::Clone ()
{
  return new csGridBagConstraint (*this);
}

/***** Implementation for class GridBagLayout ******/

csGridBagLayout::csGridBagLayout (csComponent *pParent)
  : csLayout2 (pParent),
    c (NULL),
    mpHorizCellInfos( NULL ),
    mpVertCellInfos( NULL )
{
  lc = &c;
}

csGridBagLayout::~csGridBagLayout ()
{
  ClearCachedData();
  CleanUpConstraints();
}

// impl. of LayoutManager

void csGridBagLayout::SuggestSize (int &sugw, int &sugh)
{
  if ( !HasCashedInfo() ) CreateMatrix();

  sugw  = CalcPrefSize( mpHorizCellInfos, mColCount, mRowCount, mColCount );
  sugh = CalcPrefSize( mpVertCellInfos,  mRowCount, mColCount, mRowCount );

  sugw += insets.left + insets.right;
  sugh += insets.top  + insets.bottom;
}

void csGridBagLayout::LayoutContainer ()
{
  int w = bound.Width ();
  int h = bound.Height ();
  int x, y;

  x = y = 0;

  x += insets.left;
  y += insets.top;

  w -= insets.left + insets.right;
  h -= insets.top  + insets.bottom;

  if (!HasCashedInfo()) CreateMatrix();

  // calculate horizontal extents and sizes of all cells
  LayoutCells (mpHorizCellInfos, mColCount, mRowCount, w,  x, mColCount);
  // now the same alg. is applied independently for vertical positions/sizes
  LayoutCells (mpVertCellInfos, mRowCount, mColCount, h, y, mRowCount);

  SetComponentLocations();
}

// impl. of LayoutManager2

void csGridBagLayout::MaximumLayoutSize (int &w, int &h)
{
  // FOR NOW::
  if (parent)
  {
    w = bound.Width ();
    h = bound.Height ();
  }
  else
    w = h = (int)(1<<31);
}

double csGridBagLayout::GetLayoutAlignmentX ()
{
  return 0;
}

double csGridBagLayout::GetLayoutAlignmentY ()
{
  return 0;
}

void csGridBagLayout::InvalidateLayout ()
{
  ClearCachedData();
  csLayout2::InvalidateLayout ();
}

/*** protected mehtods ***/

#define CELL_AT(x,y) cells[ (y) * _arrayWidth + (x) ]

int csGridBagLayout::CalcPrefSize (CellInfo* cells, int xCnt, int yCnt, int _arrayWidth)
{
  int prefered = 0;

  for (int x = 0; x != xCnt; ++x)
  {
    int maxColSize = 0;

    for (int y = 0; y != yCnt; ++y)
      if (CELL_AT(x,y).prefSize > maxColSize)
	maxColSize = CELL_AT(x,y).prefSize;
		
    prefered += maxColSize;
  }

  return prefered;
}

void csGridBagLayout::LayoutCells (CellInfo* cells, int xCnt, int yCnt,
				   int outterSize, int outterPos, int _arrayWidth)
{
  int actualSpaceUsed = 0;
  int x = 0, y = 0;

  // calculate prefered size of each column
  double* prefColSizes = new double[ xCnt ];

  for (x = 0; x != xCnt; ++x)
  {
    int prefColSize = 0;
    for (y = 0; y != yCnt; ++y)
      if (CELL_AT(x,y).prefSize > prefColSize)
	prefColSize = CELL_AT(x,y).prefSize;

    prefColSizes[x] = prefColSize;
  }

  double totalPrefSize = 0;

  for (x = 0; x != xCnt; ++x) totalPrefSize += prefColSizes[x];

  int extraSpace = (int)(outterSize - totalPrefSize);

  if (extraSpace <= 0)
  {
    // redistribute spaces to fit (i.e. shrink to) given outter-size
    int spaceUsed = 0;
    for (x = 0; x != xCnt; ++x)
    {
      int colSize = (int)( (double)outterSize * ( prefColSizes[x] / totalPrefSize ) );
      // elimniate round-off errors at the expence of last column
      if (x == xCnt - 1) colSize = outterSize - spaceUsed;
      for (int y = 0; y != yCnt; ++y)
	CELL_AT(x,y).finalSize = colSize;

      spaceUsed += colSize;
    }
    actualSpaceUsed = outterSize; // all space is cosumed when 
    // extra-space is absent
  }
  else
  {
    // defferent alg. if there is extra space available
    int hasStrechedCells = 0;
    for (y = 0; y != yCnt; ++y)
    {
      // calc weight-sum
      double totalWeight = 0;
      int x = 0;
      for (; x != xCnt; ++x)
	totalWeight += CELL_AT(x,y).weight;
      // distribute extraSpace in a row according to cell weights
      if (totalWeight > 0)
      {
	hasStrechedCells = 1;
	for (x = 0; x != xCnt; ++x)
	  CELL_AT(x,y).extraSpace = extraSpace * ( CELL_AT(x,y).weight / totalWeight );
      }
      else
	for (x = 0; x != xCnt; ++x)
	  CELL_AT(x,y).extraSpace = 0;
    }
    // calc total extra space consumed by each column
    double* colSpaces = new double[ xCnt ];
    double sum = 0;
    for (x = 0; x != xCnt; ++x)
    {
      double total = 0;
      for (y = 0; y != yCnt; ++y)
	total += CELL_AT(x,y).extraSpace;

      colSpaces[x] = total;
      sum += total;
    }

    // equation should hold true : "extraSpace * yCnt == sum"
    // now redistribute extra space among all cesll in each columns using, 
    // giving each column a fraction of space which corresponds to column's 
    // "totalSpace" value's relation with the "sum"

    int spaceUsed = 0; 
    actualSpaceUsed = 0;
    for (x = 0; x != xCnt; ++x)
    {
      int extraSpaceForCol = (int)( extraSpace * ( colSpaces[x] / sum ) );
      // elimniate round-off errors at the expence of last column
      if (x == xCnt - 1 && hasStrechedCells) 
	extraSpaceForCol = extraSpace - spaceUsed;
      int spaceForCol = (int)(prefColSizes[x] + extraSpaceForCol);
      actualSpaceUsed += spaceForCol;
      for (y = 0; y != yCnt; ++y)
	CELL_AT(x,y).finalSize = spaceForCol;
      spaceUsed += extraSpaceForCol;
    }
    delete [] colSpaces;
  } // end of else {....}

  // now do positioning of cells and components inside of them
  // (at this point, "actualSpaceUsed <= outterSpace" should be true)

  int curPos = (outterSize - actualSpaceUsed)/2; // center grid w/respect to bounds of outter component
  
  for (x = 0; x != xCnt; ++x)
  {
    for (y = 0; y != yCnt; ++y)
    {
      CellInfo& cell = CELL_AT(x,y);
      if (cell.comp != NULL)
      {
	cell.finalPos = curPos;
	int compSize = 0;
	int compPos  = curPos;

	// account for spaning multiple cells
	for( int i = 0; i != cell.cellSpan && x + i < xCnt; ++i )
	  compSize += CELL_AT(x+i,y).finalSize;
	// align compoenent within cell's display area

	compPos  += cell.leftInset;
	compSize -= cell.leftInset + cell.rightInset;

	cell.finalCompPos  = compPos;
	cell.finalCompSize = compSize;

	if (!cell.fill && cell.prefCompSize < compSize)
	{
	  cell.finalCompSize = cell.prefCompSize;
	  if (cell.anchor == csGridBagConstraint::_LEFT)
	    cell.finalCompPos  = compPos;
	  else
	    if (cell.anchor == csGridBagConstraint::_CENTER)
	      cell.finalCompPos = compPos + ( compSize - cell.prefCompSize ) / 2;
	    else
	      if (cell.anchor == csGridBagConstraint::_RIGHT)
		cell.finalCompPos = compPos + ( compSize - cell.prefCompSize );
	}
	cell.finalCompPos += outterPos;
      } // end of if(...)
    } // end of for(...)

    curPos += CELL_AT(x,0).finalSize;

  } // end of for(..)

  delete [] prefColSizes;
}

void csGridBagLayout::CleanUpConstraints ()
{
  vConstraints.DeleteAll ();
  vConstraints.SetLength (0);
}

void csGridBagLayout::InitializeCellArray (CellInfo* cells, int size)
{
  for (int i = 0; i != size; ++i)
  {
    memset (cells+i, 0, sizeof(CellInfo));
    cells[i].weight = 0; // doubles cannot be zero'ed with memset(..)
  }
}

void csGridBagLayout::InitCellFromHolder (CellHolder& holder)
{
  CellInfo& hCell = mpHorizCellInfos[ holder.y * mColCount + holder.x ]; // note the trick...
  CellInfo& vCell = mpVertCellInfos [ holder.x * mRowCount + holder.y ]; // -/-

  csGridBagConstraint& c = *holder.constr;

  hCell.comp = c.comp;
  vCell.comp = c.comp;

  if ( c.fill == csGridBagConstraint::BOTH || 
       c.fill == csGridBagConstraint::HORIZONTAL ) hCell.fill = 1;

  if ( c.fill == csGridBagConstraint::BOTH || 
       c.fill == csGridBagConstraint::VERTICAL ) vCell.fill = 1;

  hCell.leftInset  = c.insets.left;
  hCell.rightInset = c.insets.right;
  vCell.leftInset  = c.insets.top;
  vCell.rightInset = c.insets.bottom;

  hCell.pad = c.ipadx;
  vCell.pad = c.ipady;

  hCell.prefCompSize = (int)(c.mPrefCompSize.x  + 2*hCell.pad);
  vCell.prefCompSize = (int)(c.mPrefCompSize.y + 2*vCell.pad);

  hCell.prefSize = hCell.prefCompSize + hCell.leftInset + hCell.rightInset;
  vCell.prefSize = vCell.prefCompSize + hCell.leftInset + hCell.rightInset;

  if ( holder.isFirstCellForComp )
  {
    hCell.cellSpan = holder.actualWidth;
    vCell.cellSpan = holder.actualHeight;

    // non-zero weights are applied to the last 
    // cell of the area covered by component 
    // (this is how AWT's csGridBagLayout behaves!)

    int x = holder.x + holder.actualWidth  - 1;
    int y = holder.y + holder.actualHeight - 1;

    CellInfo& hCell1 = mpHorizCellInfos[ y * mColCount + x ]; // note the trick...
    CellInfo& vCell1 = mpVertCellInfos [ x * mRowCount + y ]; // -/-

    hCell1.weight = holder.weightx;
    vCell1.weight = holder.weighty;
  }
  // otherwise cellSpan are 0 for intermediate cells

  // adjust alginment for the horizontal-info carrying cell's

  if (c.anchor == csGridBagConstraint::NORTHWEST ||
      c.anchor == csGridBagConstraint::WEST      ||
      c.anchor == csGridBagConstraint::SOUTHWEST ) 
    hCell.anchor = csGridBagConstraint::_LEFT;

  if (c.anchor == csGridBagConstraint::NORTH  ||
       c.anchor == csGridBagConstraint::CENTER ||
       c.anchor == csGridBagConstraint::SOUTH  ) 
    hCell.anchor = csGridBagConstraint::_CENTER;

  if (c.anchor == csGridBagConstraint::NORTHEAST ||
      c.anchor == csGridBagConstraint::EAST      ||
      c.anchor == csGridBagConstraint::SOUTHEAST ) 
    hCell.anchor = csGridBagConstraint::_RIGHT;

  // adjust alginment for the vertical-info carrying cell's

  if (c.anchor == csGridBagConstraint::NORTHWEST ||
      c.anchor == csGridBagConstraint::NORTH     ||
      c.anchor == csGridBagConstraint::NORTHEAST ) 
    vCell.anchor = csGridBagConstraint::_LEFT;

  if (c.anchor == csGridBagConstraint::WEST   ||
      c.anchor == csGridBagConstraint::CENTER ||
      c.anchor == csGridBagConstraint::EAST   ) 
    vCell.anchor = csGridBagConstraint::_CENTER;

  if ( c.anchor == csGridBagConstraint::SOUTHWEST ||
       c.anchor == csGridBagConstraint::SOUTH     ||
       c.anchor == csGridBagConstraint::SOUTHEAST ) 
    vCell.anchor = csGridBagConstraint::_RIGHT;
}

long csGridBagLayout::GetCellCode (int x, int y)
{
  return (long)( x << 8 | y );
}

void csGridBagLayout::CreateMatrix ()
{
#define MAX_GRID_WIDTH  100
#define MAX_GRID_HEIGHT 100

  ClearCachedData();

  CellHolderArrayT holders;
  csHashMap  usedCellsHash (8087);

  mColCount = 0;
  mRowCount = 0;

  int nextX = 0;
  int nextY = 0;

  // creating cells for all added components according
  // to the info in their constraints

  int i = 0;
  for (; i != vConstraints.Length(); ++i)
  {
    int w=0, h=0;
    csGridBagConstraint& c = *(csGridBagConstraint*)vConstraints.Get (i);
    c.comp->SuggestSize (w, h);
    c.mPrefCompSize.Set (w, h);

    if (c.gridx != csGridBagConstraint::RELATIVE ) nextX = c.gridx;
    if (c.gridy != csGridBagConstraint::RELATIVE ) nextY = c.gridy;

    // TBD:: handle situations when grix - give, but gridy is RELATIVE, 
    //       and v.v. (should use vert/horiz scanning for not-used cells)

    while (usedCellsHash.Get (GetCellCode (nextX,nextY)) != NULL)
      ++nextX;

    int width  = c.gridwidth;
    int height = c.gridheight;

    // add this stage, we treat relavtive/REMAINDER
    // sizes as equal to 1, because we cannot yet
    // predict the actual expansion of the coponent

    if ( width == csGridBagConstraint::RELATIVE ||
	 width == csGridBagConstraint::REMAINDER )
      width = 1;
    if ( height == csGridBagConstraint::RELATIVE ||
	 height == csGridBagConstraint::REMAINDER )
      height = 1;

    /*
      // OLD STUF::
      // distribute weigths uniformly among all
      // cells which are covered by current compoenent

      //double weightx = c.weightx / (double)width;
      //double weighty = c.weighty / (double)height;
    */

    // create cells for the area covered by the component

    CellHolder* pHolder = NULL;

    for (int xofs = 0; xofs != width; ++xofs)
      for( int yofs = 0; yofs != height; ++yofs )
      {
	pHolder = new CellHolder();
	pHolder->constr = &c;
	pHolder->isFirstCellForComp = ( xofs == 0 && yofs == 0 );

	pHolder->x = nextX + xofs;
	pHolder->y = nextY + yofs;

	pHolder->weightx = c.weightx;
	pHolder->weighty = c.weighty;

	pHolder->gridwidth  = c.gridwidth;
	pHolder->gridheight = c.gridheight;

	holders.Push (pHolder);

	usedCellsHash.Put (GetCellCode( nextX + xofs, nextY + yofs ), 
			   (csHashObject)pHolder);
      }

    if (c.gridwidth == csGridBagConstraint::REMAINDER)
      // mark all possible reminding cells in the row as used
      for (int x = nextX+width; x < MAX_GRID_WIDTH; ++x)
	usedCellsHash.Put (GetCellCode (x, nextY), (csHashObject)pHolder);

    if (c.gridheight == csGridBagConstraint::REMAINDER)
      // mark all possible eminding cells in the column as used
      for (int y = nextY+height; y < MAX_GRID_HEIGHT; ++y)
	usedCellsHash.Put (GetCellCode (nextX, y), (csHashObject)pHolder);
    // adjust estimated dimensions of the matrix (grid)

    if (nextX + width  > mColCount) 
      mColCount = nextX + width;

    if (nextY + height > mRowCount) 
      mRowCount = nextY + height;

    // move on to next cell

    if (c.gridwidth == csGridBagConstraint::REMAINDER) 
    { 
      nextX = 0; ++nextY; 
    }
    else
    {
      if (c.gridwidth == csGridBagConstraint::RELATIVE) nextX += 1;
      else nextX += width;
    }

  }  // end of for(...)

  // now actually create matrix

  int sz = mColCount*mRowCount;

  mpHorizCellInfos = new CellInfo[ sz ];
  mpVertCellInfos  = new CellInfo[ sz ];
  InitializeCellArray (mpHorizCellInfos, sz);
  InitializeCellArray (mpVertCellInfos,  sz);

  // and fill in cells with info

  for (i = 0; i != holders.Length(); ++i)
  {	
    CellHolder& h = *holders[i];
    if (h.isFirstCellForComp)
     {
       // now set-up actual gridwidth, and gridheigh parameters
       int actualWidth  = h.constr->gridwidth;
       int actualHeight = h.constr->gridheight;
       int x = h.x, y = h.y;

       // extend widths/heights if given as relative/REMAINDER by 
       // traversing grid until end is reached or cell occupied by 
       // anther component is encountered

       if ( h.constr->gridwidth == csGridBagConstraint::RELATIVE ||
	    h.constr->gridwidth == csGridBagConstraint::REMAINDER )
       {
	 // TBD:: comments.. this is how AWT's GridBagLayout behaves...
	 ++x;
	 while (x < mColCount)
	 {
	   CellHolder* pHolder = (CellHolder*)usedCellsHash.Get (GetCellCode(x,y));
	   if (!pHolder || (pHolder && pHolder->constr != h.constr) ) 
	     break;
	   ++x;
	 }
	 actualWidth = x - h.x;
       }

       if ( h.constr->gridheight == csGridBagConstraint::RELATIVE ||
	    h.constr->gridheight == csGridBagConstraint::REMAINDER )
       {
	 ++y;
	 while (y < mRowCount)
	 {
	   CellHolder* pHolder = (CellHolder*)usedCellsHash.Get (GetCellCode (x,y));
	   if (!pHolder || (pHolder && pHolder->constr != h.constr))
	     break;
	   ++y;
	 }
	 actualHeight = y - h.y;
       }

       h.actualWidth  = actualWidth;
       h.actualHeight = actualHeight;

       // split info contained in holder and constraints among two cell objects:
       // one - carrying vertical info, another -carrying horizontal info

       InitCellFromHolder (*holders[i]);
     }
  }
}

void csGridBagLayout::ClearCachedData ()
{
  if ( mpHorizCellInfos ) delete [] mpHorizCellInfos;
  if ( mpVertCellInfos  ) delete [] mpVertCellInfos;

  mpHorizCellInfos = NULL;
  mpVertCellInfos  = NULL;
}

bool csGridBagLayout::HasCashedInfo ()
{
  return mpHorizCellInfos != NULL;
}

void csGridBagLayout::SetComponentLocations ()
{
  for (int y = 0; y != mRowCount; ++y)
    for (int x = 0; x != mColCount; ++x)
    {
      CellInfo& hCell = mpHorizCellInfos[ y * mColCount + x ]; // note the trick...
      CellInfo& vCell = mpVertCellInfos [ x * mRowCount + y ]; // -/-
      if (hCell.comp != NULL)
      {
	hCell.comp->SetRect (hCell.finalCompPos, vCell.finalCompPos,
			     hCell.finalCompPos + hCell.finalCompSize, 
			     vCell.finalCompPos + vCell.finalCompSize );
      }
    }
} 
