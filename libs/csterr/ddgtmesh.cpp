/*
    Copyright (C) 1997, 1998, 1999 by Alex Pfaffe
	(Digital Dawn Graphics Inc)
  
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
  
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
  
    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

//
#ifdef DDG
#include "ddgtmesh.h"
#include "ddgbtree.h"
#else
#include "sysdef.h"
#include "csterr/ddgtmesh.h"
#include "csterr/ddgbtree.h"
#endif
// Size should be (# of tiles + #of triangles/frame ) *3 + 10 %

ddgVCache::ddgVCache( void )
{
	_size = 0;
	_used = 0;
}

void ddgVCache::init (unsigned int size )
{
	_cache = new ddgVector3[size];
	ddgAsserts(_cache,"Failed to Allocate memory");
	ddgMemorySet(ddgVector3,size);
	_size = size;
	_used = 0;
}
///
ddgVCache::~ddgVCache( void )
{
	delete _cache;
}


// ----------------------------------------------------------------------
//const unsigned int TBinMesh_size = 64;	// 64 seems to be a good size.
ddgHeightMap *heightMap = 0;
ddgTBinMesh::ddgTBinMesh( ddgHeightMap * h )
{
	heightMap = h;
	_bintreeMax = 2*(heightMap->cols()-1)/(ddgTBinMesh_size) * (heightMap->rows()-1)/(ddgTBinMesh_size);
	typedef ddgTBinTree *ddgTBinTreeP;
	_bintree = new ddgTBinTreeP[_bintreeMax];
	ddgAsserts(_bintree,"Failed to Allocate memory");
	ddgMemorySet(ddgTBinTreeP,_bintreeMax);
	_minDetail = 3000;
	_maxDetail = 3200;
	_absMaxDetail = 5000;
	// Note Z axis is negative into the screen (Right Handed coord sys).
#ifdef DDG
	_farClip = -150;
#else
	_farClip = 150;
#endif
	_nearClip = 0;
	_progDist = _farClip/15;
	_merge = true;
	// Allocate memory for transformed vertices.
	_vcache.init(((_bintreeMax + _absMaxDetail) * 3 * 11)/10);  // (# of tiles + #of triangles/frame ) *3 + 10 %
	_tcache.init(((_bintreeMax + _absMaxDetail) * 11)/10);  // (# of tiles + #of triangles/frame ) + 10 %

	_maxLevel = (unsigned int) (log(ddgTBinMesh_size)/log(2.0) * 2); // Could be constant.
	_triNo = (unsigned int)pow(2,(_maxLevel)); // Inside points can be shared.
	_leafTriNo = _triNo / 2;
	stri = new ddgMSTri[_triNo+2];
	ddgAsserts(stri,"Failed to Allocate memory");
	ddgMemorySet(ddgMSTri,_triNo+2);

	// Queues to hold the visible, clipped and sorted visible triangles.
#ifdef HASH
	_qs = new ddgHashTable(); 
	_qm = new ddgHashTable();
	ddgAsserts(_qs && _qm,"Failed to Allocate memory");

	ddgMemorySet(ddgHashTable,2);
	// By default the iterators support 1000 levels of nesting.
	_qsi = new ddgHashIterator(_qs);
	_qmi = new ddgHashIterator(_qm);
	ddgAsserts(_qsi && _qmi,"Failed to Allocate memory");
	ddgMemorySet(ddgHashIterator,2);
#else
	_qs = new ddgSplayTree( );
	_qm = new ddgSplayTree( );
	ddgAsserts(_qs && _qm,"Failed to Allocate memory");
	ddgMemorySet(ddgSplayTree,2);
	// By default the iterators support 1000 levels of nesting.
	_qsi = new ddgSplayIterator(_qs);
	_qmi = new ddgSplayIterator(_qm);
	ddgAsserts(_qsi && _qmi,"Failed to Allocate memory");
	ddgMemorySet(ddgSplayIterator,2);
#endif
	unsigned int i;
	for (i = 0; i < _bintreeMax; i++ )
		_bintree[i] = NULL;
	// Initialize the queue on the 1st frame.
	clearSQ();
	clearMQ();

	// Dummy entries.
	stri[_triNo].row = ddgTBinMesh_size;
	stri[_triNo].col = 0;
	stri[_triNo+1].row = 0;
	stri[_triNo+1].col = ddgTBinMesh_size;
	// Top level triangles.
	stri[0].row = 0;
	stri[0].col = 0;
	stri[1].row = (ddgTBinMesh_size)/2;
	stri[1].col = (ddgTBinMesh_size)/2;
    initVertex(1,0,_triNo,_triNo+1,1);
	initNeighbours();

	// Should perhaps be generated by each BinTree or passed in with the image.
	_normalLUT = NULL;
#ifdef DDG
	generateNormals();
#endif
	/// Total number of triangles rendered by this camera.
	_triCount = 0;
	/// Total number of priorities calculated by this camera.
	_priCount = 0;
	/// Total number of queue insertions by this camera.
	_insCount = 0;
	/// Total number of queue removals by this camera.
	_remCount = 0;
	/// Total number of queue updates by this camera.
	_movCount = 0;
	_visCount = 0;
	_resetCount = 0;

}

unsigned short *_normalidx = 0;
ddgTBinMesh::~ddgTBinMesh(void)
{
	delete stri;
	delete _qs;
	delete _qm;
	delete _bintree;
	delete[] _normalLUT;
	delete[] _normalidx;
}


bool ddgTBinMesh::generateNormals(void)
{
    int ri, rmax = heightMap->rows();
    int ci, cmax = heightMap->cols();
	// Generate lookup table.
	float x,y,z;
	_normalLUT = new ddgVector3[256*256];
	ddgAsserts(_normalLUT,"Failed to Allocate memory");
	ddgMemorySet(ddgVector3,256*256);
	float s = 1.0/sqrt(2.0);
	for (ri = 0; ri < 256; ri++)
	{
		if (ri==0)
		{
			z = s;
		}
		else
		{
			z = s*cos(ddgAngle::degtorad((float)ri/256.0*180.0));
		}
		for (ci = 0; ci < 256; ci++)
		{
			if (ci==0)
			{
				x = s;
			}
			else
			{
				x = s*cos(ddgAngle::degtorad((float)ci/256.0*180.0));
			}
			y = sqrt(1 - z*z - x*x);
			_normalLUT[ri*256+ci] = ddgVector3(x,y,z);
#ifdef DDG
			_normalLUT[ri*256+ci].normalize();
#else
			_normalLUT[ri*256+ci].Normalize();
#endif
		}
	}	// Set up the bintrees managed by this mesh.
	_normalidx = new unsigned short[rmax*cmax];
	ddgAsserts(_normalidx,"Failed to Allocate memory");
	ddgMemorySet(unsigned short,rmax*cmax);
 	ddgVector3 n, v[9];
	const ddgVector3 *vp[9];
	vp[0] = &v[0];
	vp[1] = &v[1];
	vp[2] = &v[2];
	vp[3] = &v[3];
	vp[4] = &v[4];
	vp[5] = &v[5];
	vp[6] = &v[6];
	vp[7] = &v[7];
	vp[8] = &v[8];
	for ( ri = 0; ri < rmax; ri++)
	{
		for ( ci = 0; ci < cmax; ci++)
		{
            // Ignore points which lie outside the grid.
			
			v[0] = ddgVector3(ri?ri-1:ri,		heightMap->getf(ri?ri-1:ri,ci?ci-1:ci),ci?ci-1:ci);
			v[1] = ddgVector3(ri?ri-1:ri,		heightMap->getf(ri?ri-1:ri,ci  ),ci);
			v[2] = ddgVector3(ri?ri-1:ri,		heightMap->getf(ri?ri-1:ri,ci<cmax-1?ci+1:ci),ci<cmax-1?ci+1:ci);
			v[3] = ddgVector3(ri,				heightMap->getf(ri,  ci<cmax-1?ci+1:ci),ci<cmax-1?ci+1:ci);
			v[4] = ddgVector3(ri<rmax-1?ri+1:ri,	heightMap->getf(ri<rmax-1?ri+1:ri,ci<cmax-1?ci+1:ci),ci<cmax-1?ci+1:ci);
			v[5] = ddgVector3(ri<rmax-1?ri+1:ri,	heightMap->getf(ri<rmax-1?ri+1:ri,ci  ),ci);
			v[6] = ddgVector3(ri<rmax-1?ri+1:ri,	heightMap->getf(ri<rmax-1?ri+1:ri,ci?ci-1:ci),ci?ci-1:ci);
			v[7] = ddgVector3(ri,				heightMap->getf(ri,  ci?ci-1:ci),ci?ci-1:ci);
			v[8] = ddgVector3(ri,				heightMap->getf(ri,  ci  ),ci);
#ifdef DDG
			n.normal(vp);
#else
			return ddgFailure; // Not implemented in csVector.
#endif
            // Since a grid cannot fold on itself there can be no
            // negative normals.
			ddgAsserts((n[1]>=0.0),"Invalid normal.");
			unsigned short nindx = 0;
			float dx = ddgAngle::radtodeg(acos(n[0]));
			float dz = ddgAngle::radtodeg(acos(n[2]));
			int rl, cl;
			rl = int(256*dz/180);
			cl = int(256*dx/180);
			nindx = rl*256+cl;
			_normalidx[ri*cmax+ci] = nindx;
			// The choosen index should have x and z components which match v[0] and v[2]
			// $TODO this is not yet correct because the y component calculation in the
			//  normal map is not right.
		}
#ifdef DDG
		if (ri%10 == 0) cerr << '.';
#endif
	}


#ifdef DDG
	cerr << '\n';
#endif
	return ddgSuccess;
}
/// Function to add a bintree.
void ddgTBinMesh::addBinTree( ddgTBinTree *bt )
{
	unsigned int i = 0;
	while (i < _bintreeMax && _bintree[i] != NULL) i++;
	if (i < _bintreeMax)
	{
		_bintree[i] = bt;
	}
}
/// Function to remove a bintree.
void ddgTBinMesh::removeBinTree( ddgTBinTree *bt )
{
	unsigned int i = 0;
	while (i < _bintreeMax && _bintree[i] != bt) i++;
	if (i < _bintreeMax)
	{
		_bintree[i] = NULL;
	}
}

/** Initialize the bintree structure including
 *  the wedges and other mesh values.
 */
bool ddgTBinMesh::init( void )
{

    ddgTBinTree	*bintree1 = NULL,
				*bintree2 = NULL;
	// Created the enough bintrees to cover the terrain.
	unsigned int i, j;
	_nc = (heightMap->cols()-1)/ddgTBinMesh_size;
	_nr = (heightMap->rows()-1)/ddgTBinMesh_size;
#ifdef DDG
	cerr << "Creating " << _nc << " x " << _nr << " Tiles" << endl;
#endif

	ddgTreeIndex index = 0;
	for (i = 0; i < _nr; i++)
	{
		for (j = 0; j < _nc; j++)
		{
			bintree1 = new ddgTBinTree(this,index,heightMap,_normalidx,i*ddgTBinMesh_size,j*ddgTBinMesh_size);
			ddgMemorySet(ddgTBinTree,1);
			addBinTree(bintree1);
			index++;
			bintree2 = new ddgTBinTree(this,index,heightMap,_normalidx,(i+1)*ddgTBinMesh_size,(j+1)*ddgTBinMesh_size,true);
			ddgMemorySet(ddgTBinTree,1);
			addBinTree(bintree2);
			index++;
		}
	}

	// Define relationship between meshes.
	for (i = 0; i < _nr; i++)
	{
		for (j = 0; j < _nc; j++)
		{
			_bintree[2*(i*_nc+j)]->pNeighbourDiag( _bintree[2*(i*_nc+j)+1]);
			_bintree[2*(i*_nc+j)]->pNeighbourLeft( (j==0)? NULL: _bintree[2*(i*_nc+j-1)+1]);
			_bintree[2*(i*_nc+j)]->pNeighbourTop( (i==0)? NULL: _bintree[2*((i-1)*_nc+j)+1]);
			//
			_bintree[2*(i*_nc+j)+1]->pNeighbourDiag( _bintree[2*(i*_nc+j)]);
			_bintree[2*(i*_nc+j)+1]->pNeighbourLeft( (j+1==_nc)? NULL: _bintree[2*(i*_nc+j+1)]);
			_bintree[2*(i*_nc+j)+1]->pNeighbourTop( (i+1==_nr)? NULL: _bintree[2*((i+1)*_nc+j)]);
		}
	}

	for (i = 0; i < _bintreeMax; i++)
	{
		if (_bintree[i])
		{
			_bintree[i]->init();
		}
	}
#ifdef DDG
	cerr << endl << "Calculating wedgies:";
#endif
    return ddgSuccess;
}

/**
 *  Initialize the vertex and row/col data of the STri.
 *  Passed in are the triangles which carry the points
 *  that describe the current triangle.
 *  va is immediate parent.
 *  v1 and v0 are the left and right vertices of va. 
 */

void ddgTBinMesh::initVertex( unsigned int level,
					 ddgTriIndex va,	// Top
					 ddgTriIndex v0,	// Left
					 ddgTriIndex v1,	// Right
					 ddgTriIndex vc)	// Centre
{
	// Initialize row and col for this triangle.
	stri[vc].row = (stri[v0].row + stri[v1].row) / 2;
	stri[vc].col = (stri[v0].col + stri[v1].col) / 2;

	// See if we are not at the bottom.
	if (level < _maxLevel)
	{
		initVertex(level+1,vc,va,v0,ddgTBinTree::left(vc)),
		initVertex(level+1,vc,v1,va,ddgTBinTree::right(vc));
	}
	stri[vc].v0 = v0;
	stri[vc].v1 = v1;

}

// Constant used for identifying neighbour.
const	unsigned int ddgNINIT = 0xFFFFFFFF;

// Precompute the neighbour field, this is used for merge diamonds.
// This function can now initialize in constant time, there is an algoritm!
void ddgTBinMesh::initNeighbours( void )
{
	// Find the neighbours of all triangles.
	unsigned int t, b, k = 2, lk = 1, l = 0, klk, kk, kt;

	// initialize all neighbours to ddgNINIT.
	for (t = 0; t < _triNo+2; t++)
		stri[t].neighbour = ddgNINIT;

	stri[1].neighbour = 1;
	stri[0].neighbour = 0;
	stri[_triNo].neighbour = _triNo;
	stri[_triNo+1].neighbour = _triNo+1;
#ifdef DDG
	cerr << "Computing neighbour...";
#endif
	// Find the neighbour of each tri.
	while ((++l) < _maxLevel)
	{
        b = lk;
		klk = k+lk;
		kk = k+k;
		for (t = 0; t < lk; t++) // Only do half because of symmetry.
		{
			kt = k+t;
			if (stri[kt].neighbour != ddgNINIT)
			{
				continue;
			}
            else if (l > 2 && l % 2 == 1) // Odd levels are a double copy of prev even level.
            {
                stri[kt].neighbour = stri[lk+t].neighbour + b;
                stri[klk+t].neighbour = stri[lk+t].neighbour + lk + b;
            }
    	    // Check Edge cases
			else if ((b = edge(kt)))
			{
				if (b==3) // Diagonal.
				{
				stri[kt].neighbour = kk-t-1;
				stri[kk-t-1].neighbour = kt;
				}
				else
				{
				stri[kt].neighbour = klk-t-1;
				stri[kk-t-1].neighbour = klk+t;
				}
			}
			// Normal case
			else
			{
				ddgAssert(stri[kt].neighbour == ddgNINIT);
				// If the same row in the previous level was known, inherit.
                if (lk && t < lk && !edge(lk + t))
                {
					stri[kt].neighbour = stri[lk + t].neighbour + lk;
					stri[stri[kt].neighbour].neighbour = kt;
					stri[klk+t].neighbour = stri[lk + t].neighbour + k;
					stri[lk+stri[kt].neighbour].neighbour = klk + t;
                }
                else // It is a new neighbour.
                {
					b = k - 1 - t;
					ddgAssert ( stri[kt].col == stri[k+b].col
					  && stri[kt].row == stri[k+b].row);
					stri[kt].neighbour = k+b;
					stri[k+b].neighbour = kt;
                }
			}
		}
        lk = k;
		k = kk;  // 2^n
	}
}


bool ddgTBinMesh::calculate( void )
{
	// Dont do anything if we didnt move and the current frame is done.
	if (!_dirty && !_balanceCount)
		return false;

	unsigned int i = 0;

	// Reset the world transformed vertex cache.
	_vcache.reset();
	// Calculate visibility info for all triangle currently in the mesh
	// at the current camera position.
	static bool lastMerge = false;
	if (!lastMerge || !merge())
	{
		clearSQ();
		clearMQ();
	}

	while (i < _bintreeMax)
	{
		if (_bintree[i])
		{
			_bintree[i]->tri(triNo())->reset();
			_bintree[i]->tri(triNo()+1)->reset();
			_bintree[i]->tri(0)->reset();
			if (!lastMerge || !merge())
			{
				// Top level triangles.
				_bintree[i]->tri(0)->_state = 0;
				_bintree[i]->tri(1)->_state = 0;
				_bintree[i]->tri(triNo())->_state = 0;
				_bintree[i]->tri(triNo()+1)->_state = 0;
				_bintree[i]->tri(0)->reset();
				_bintree[i]->tri(1)->reset();
				_bintree[i]->tri(triNo())->reset();
				_bintree[i]->tri(triNo()+1)->reset();
				_bintree[i]->insertSQ(1);
			}
			_bintree[i]->visibility(1);

		}
		i++;
	}
	// Update priorities based on frustrum culling etc.
	// If a priority has changed, the triangle needs to be removed and reinserted
	i = 0;
	while (i < _bintreeMax)
	{
		if (_bintree[i])
		{
			_bintree[i]->priorityUpdate(1);
		}
		i++;
	}

	// Get the optimal number of triangles in the queue.
	_balanceCount = balanceQueue();

	lastMerge = merge();
    // Track the number of triangles produced be this calculation.
	_triCount += _tcache.size();

	_dirty = false;
	return _balanceCount ? true : false;
}

#ifdef _DEBUG
void ddgTBinMesh::verify(void)
{
	_qsi->reset();
	// Verify that all merge diamonds are present for the triangles in the split queue.
	while(!_qsi->end()) 
	{
#ifdef DDG
		ddgTriIndex tindex = indexSQ(_qsi);
		ddgAsserts(treeSQ(_qsi)->verify(tindex) == false, "Triangle missing from merge queue.");
#endif
		_qsi->next();
	}

	// Clear queue.
	return;
}
#endif
// Destroy all elements in the queue.
void ddgTBinMesh::clearSQ(void)
{
	_qsi->reset();
	while(!_qsi->end()) 
	{
		DDG_BCLEAR(treeSQ(_qsi)->tri(indexSQ(_qsi))->_state, ddgMTri::SF_SQ);
		_qsi->next();
	}

	// Clear queue.
	_qs->clear();
}

// Destroy all elements in the queue.
void ddgTBinMesh::clearMQ(void)
{
	_qmi->reset();
	while(!_qmi->end()) 
	{
		DDG_BCLEAR(treeMQ(_qmi)->tri(indexMQ(_qmi))->_state, ddgMTri::SF_SQ);
		_qmi->next();
	}
	// Clear queue.
	_qm->clear();
}

// Returns false if we are blocked from splitting any further.
bool ddgTBinMesh::splitQueue(void)
{
	bool split = false;
	// Remove the biggest item, split it.
	_qsi->reset(true);
	while (!_qsi->end())
	{
		ddgTBinTree *bt = treeSQ(_qsi);
		ddgTriIndex i = indexSQ(_qsi);
		if ( // i < _leafTriNo && // Not a leaf!
			 bt->visible(i))
		{
			bt->forceSplit(i);
			split = true;
			break;
		}
		_qsi->prev();
	} 
	return split;
}

// Returns false if we are blocked from merging any further.
bool ddgTBinMesh::mergeQueue(void)
{
	bool merged = false;
	// Remove the smallest item, merge it.
	_qmi->reset();
	while (!_qmi->end())
	{
		ddgTBinTree *bt = treeMQ(_qmi);
		ddgTriIndex i = indexMQ(_qmi);
		if ( i > 0 )	 // Not a root!
		{
			bt->forceMerge(i);
			merged = true;
			break;
		}
		_qmi->next();
	}
	return merged;
}


/* Reoptimize the mesh by splitting high priority triangles
 * and merging low priority diamonds.
 * Return the number of balance operations done.
 */
unsigned int ddgTBinMesh::balanceQueue(void)
{
	// Loops should be gone now that we have the flags to indicate if a triangle has been split/merged
	// in this cycle.
    unsigned int maxLoop = merge()?_minDetail/4 : 0xFFFF; // Don't make more than maxLoop adjustments.
	unsigned int priDiff = _minDetail/30;		// Allowable difference in priorities between queues.
	unsigned int minDiff = _minDetail-_minDetail/100; // Min # triangles to start merging.
    unsigned int count = 0;
	bool merged = true, split = true;
    // General case.
	while (merged || split)			// We are still making progress.
	{
		merged = split = false;
        // See if we need to merge
		if ( merge() && _qm->size())
		{
			// Merge the diamond of lowest priority.
			if ( _tcache.size() > _maxDetail)  // We have too many triangles.
			{
				merged = mergeQueue();
			}
			else	// Check if the queues are balanced.
			{
				_qmi->reset();
				_qsi->reset(true);
				unsigned int pm = priorityMQ(_qmi);
				unsigned int ps = prioritySQ(_qsi);
				if ((pm == 0)			// We have useless merge diamonds, merge these 1st.
										// We safely beyond the minimum # of triangles, and
										// there is significant difference between the
										// priorities of the split and merge queues.
				  ||(_tcache.size() > minDiff && (ps > pm + priDiff)) )
				{
					merged = mergeQueue();
				}
			}
		}
		// See if we need to split.
		if (!merged && _tcache.size() < _minDetail )                     // We don't have enough triangles.
		{
			// So  split the triangle of highest priority.
			split = splitQueue();
		}

		if (merged || split)
			count++;

        if ( count > maxLoop )
        {
            break;  // We are taking too long.
        }
	}

	return count;
}


void ddgTBinMesh::pos(float *x, float *z, ddgTBinTree **bt, unsigned int *r, unsigned int *c)
{
	// Check that we are within limit.
	if (*x < 0) *x = 0;
	else if (*x >= _nr * ddgTBinMesh_size) *x = _nr * ddgTBinMesh_size-1;
	if (*z < 0) *z = 0;
	else if (*z >= _nc * ddgTBinMesh_size) *z = _nc * ddgTBinMesh_size-1;
    // Find the global(mesh) column and row of this coordinate.
	unsigned int gr = (unsigned int)(*x/ddgTBinMesh_size);
	unsigned int gc = (unsigned int)(*z/ddgTBinMesh_size);
    // Find the offset in the tree and row and column within the tree.
    *x = *x - gr * ddgTBinMesh_size;
	*z = *z - gc * ddgTBinMesh_size;
    // Do we fall in to the tree across the diagonal?
	bool mirror = (*x+*z < ddgTBinMesh_size)?false:true;
    // If so calculate the row and column w.r.t. that tree.
	if (mirror)
	{
        *x = ddgTBinMesh_size - *x;
        *z = ddgTBinMesh_size - *z;
	}
    *r = (unsigned int)*x;
    *c = (unsigned int)*z;
    // Calculate the distance from the triangle origin to the coord.
    *x = *x - *r;
    *z = *z - *c;
    // Calculate the bin tree.
	*bt = _bintree[(gr * _nc + gc)*2+(mirror?1:0)];
}

float ddgTBinMesh::height(float x, float z)
{
    // Find the binTree which owns this coordinate.
    ddgTBinTree *bt = 0;
    unsigned int r, c;
    pos(&x,&z,&bt, &r, &c);
	tx = x; tz = z;
	ddgAssert(bt);
    // Find mesh height at this location.
	return bt ? bt->treeHeight(r,c,x,z) : 0.0;
}
