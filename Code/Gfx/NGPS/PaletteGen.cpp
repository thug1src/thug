
#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include <gfx/ngps/PaletteGen.h>


namespace Nx
{

#define	RED	1
#define	GREEN	2
#define BLUE	3
#define ALPHA	4

#define MAXDEPTH 5
#define MAXCOLOR 256

#define ColorRealLengthSq(a)	((a)->red*(a)->red + (a)->green*(a)->green + (a)->blue*(a)->blue + (a)->alpha*(a)->alpha)
#define	ColorRealFromColorInt(o,i)										\
{                                                                        \
    (o)->red   =                                                         \
        (((float)(((i)->r))) * (   (float)((1.0/255.0))));           \
    (o)->green =                                                         \
        (((float)(((i)->g))) * ( (float)((1.0/255.0))));           \
    (o)->blue  =                                                         \
        (((float)(((i)->b))) * (  (float)((1.0/255.0))));           \
    (o)->alpha =                                                         \
        (((float)(((i)->a))) * ( (float)((1.0/255.0))));           \
}                                                                        \

#define ColorRealScale(o,a,scale)										\
{                                                                        \
    (o)->red   = (((a)->red) * (   scale));                              \
    (o)->green = (((a)->green) * ( scale));                              \
    (o)->blue  = (((a)->blue) * (  scale));                              \
    (o)->alpha = (((a)->alpha) * ( scale));                              \
}                                                                        \

#define ColorRealAdd(o,a,b)                                        \
{                                                                        \
    (o)->red   = (((a)->red) + (   (b)->red));                           \
    (o)->green = (((a)->green) + ( (b)->green));                         \
    (o)->blue  = (((a)->blue) + (  (b)->blue));                          \
    (o)->alpha = (((a)->alpha) + ( (b)->alpha));                         \
}                                                                        \

#define ColorRealSub(o,a,b)                                        \
{                                                                        \
    (o)->red   = (((a)->red) - (   (b)->red));                           \
    (o)->green = (((a)->green) - ( (b)->green));                         \
    (o)->blue  = (((a)->blue) - (  (b)->blue));                          \
    (o)->alpha = (((a)->alpha) - ( (b)->alpha));                         \
}                                                                        \

typedef uint32 OctantMap;

/* local */
static OctantMap    *p_splice = NULL;

/*************************************************************************/
static LeafNode    *
InitLeaf(LeafNode * Leaf)
{
    Dbg_Assert( Leaf );

    Leaf->palIndex = 0;
    Leaf->weight = 0.0f;
    Leaf->ac.red = 0.0f;
    Leaf->ac.green = 0.0f;
    Leaf->ac.blue = 0.0f;
    Leaf->ac.alpha = 0.0f;
    Leaf->m2 = 0.0f;
    
	return Leaf;
}

/*************************************************************************/
static BranchNode  *
InitBranch(BranchNode * Branch)
{
    int                 i;

    Dbg_Assert( Branch );

    for (i = 0; i < 16; i++)
    {
        Branch->dir[i] = (OctNode *)NULL;
    }

    return Branch;
}

/*************************************************************************/
static OctNode     *
CreateCube( void )
{
    OctNode            *cube;

	cube = (OctNode*) Mem::Calloc( 1, sizeof( OctNode ));
    return(cube);
}

/*************************************************************************/
static              OctantMap
GetOctAdr(Image::RGBA * c)
{
    int                 cs = 8 - MAXDEPTH;

    Dbg_Assert(c);

    return((p_splice[c->r >> cs] << 3) | (p_splice[c->g >> cs] << 2) |
             (p_splice[c->b >> cs] << 1) | (p_splice[c->a >> cs] << 0));
}

/*************************************************************************/
static OctNode     *
AllocateToLeaf(PalQuant * pq, OctNode * root, OctantMap Octs, int depth)
{

    Dbg_Assert(pq);
    Dbg_Assert(root);

    /* return leaf */
    if (depth == 0)
    {
        return(root);
    }

    /* populate branch */
    if (!root->Branch.dir[Octs & 15])
    {
        OctNode            *node;

        node = CreateCube();//(pq->cubefreelist);
        root->Branch.dir[Octs & 15] = node;
        if (depth == 1)
        {
            InitLeaf(&node->Leaf);
        }
        else
        {
            InitBranch(&node->Branch);
        }
    }

    return(AllocateToLeaf
             (pq, root->Branch.dir[Octs & 15], Octs >> 4, depth - 1));
}

/*************************************************************************/
void
PalQuantAddImage(PalQuant *pq, uint8 *img, int width, int height, int image_bpp, float weight)
{
    int     i, size;//, stride;
    //unsigned char		*pixels;    

    Dbg_Assert(pq);
    Dbg_Assert(img);

    //pixels = (unsigned char*) texture->buf;//GetTexelData( 0 );    
	size = width * height;

    switch (image_bpp / 8)
    {
		case 3:
		{			
#if 0
			Image::RGBA         color;
            ColorReal         rColor;
            OctNode            *leaf;
            OctantMap           Octs;

			color24* colors= (color24*)img;
			for( i = 0; i < size; i++ )
			{
				color.red = colors[i].r;
				color.green = colors[i].g;
				color.blue = colors[i].b;
				color.alpha = 255;			
			
				/* build down to leaf */
				Octs = GetOctAdr(&color);
				leaf = AllocateToLeaf(pq, pq->root, Octs, MAXDEPTH);

				ColorRealFromColorInt(&rColor, &color);
				leaf->Leaf.weight += weight;
				ColorRealScale(&rColor, &rColor, weight);
				ColorRealAdd(&leaf->Leaf.ac, &leaf->Leaf.ac,
							  &rColor);
				leaf->Leaf.m2 += weight*ColorRealLengthSq(&rColor);
			}
#endif
			Dbg_Assert(0);
			break;
		}
		case 4:
		{
			Image::RGBA         color;
            ColorReal         rColor;
            OctNode            *leaf;
            OctantMap           Octs;

			uint32* p_colors= (uint32*)img;
			for( i = 0; i < size; i++ )
			{
				color = *((Image::RGBA *) &p_colors[i]);
			
				/* build down to leaf */
				Octs = GetOctAdr(&color);
				leaf = AllocateToLeaf(pq, pq->root, Octs, MAXDEPTH);

				ColorRealFromColorInt(&rColor, &color);
				leaf->Leaf.weight += weight;
				ColorRealScale(&rColor, &rColor, weight);
				ColorRealAdd(&leaf->Leaf.ac, &leaf->Leaf.ac,
							  &rColor);
				leaf->Leaf.m2 += weight*ColorRealLengthSq(&rColor);
			}
			break;
		}

        default:
			break;
    }    
}

/*************************************************************************/

/*************************************************************************/
static void
assignindex(OctNode * root, Image::RGBA * origin, int depth, box * region,
            int palIndex)
{
    int                 width, dr, dg, db, da, dR, dG, dB, dA;
    int                 i;

    Dbg_Assert(origin);
    Dbg_Assert(region);

    if (!root)
        return;

    width = 1 << depth;

    dr = origin->r - region->col1.r;
    dg = origin->g - region->col1.g;
    db = origin->b - region->col1.b;
    da = origin->a - region->col1.a;
    if (dr >= 0 || dg >= 0 || db >= 0 || da >= 0)
    {
        return;
    }

    dR = region->col0.r - origin->r;
    dG = region->col0.g - origin->g;
    dB = region->col0.b - origin->b;
    dA = region->col0.a - origin->a;
    if (dR >= width || dG >= width || dB >= width || dA >= width)
    {
        return;
    }

    /* wholly inside region and a leaf? */
    if (dr <= -width && dg <= -width && db <= -width && da <= -width)
        if (dR <= 0 && dG <= 0 && dB <= 0 && dA <= 0)
            if (depth == 0)
            {
                root->Leaf.palIndex = (unsigned char) palIndex;
                return;
            }

    /* try children */
    depth--;
    for (i = 0; i < 16; i++)
    {
        Image::RGBA              suborigin;

        suborigin.r = origin->r + (((i >> 3) & 1) << depth);
        suborigin.g = origin->g + (((i >> 2) & 1) << depth);
        suborigin.b = origin->b + (((i >> 1) & 1) << depth);
        suborigin.a = origin->a + (((i >> 0) & 1) << depth);

        assignindex(root->Branch.dir[i], &suborigin, depth, region, palIndex);
    }    
}

/*************************************************************************/

/* Assign palIndex to leaves */
static void
nAssign(int palIndex, OctNode * root, box * cube)
{
    Image::RGBA              origin;

    Dbg_Assert(root);
    Dbg_Assert(cube);

    origin.r = 0;
    origin.g = 0;
    origin.b = 0;
    origin.a = 0;
    assignindex(root, &origin, MAXDEPTH, cube, palIndex);    
}

/*************************************************************************/
static void
addvolume(OctNode * root, Image::RGBA * origin, int depth, box * region,
          LeafNode * volume)
{
    int                 width, dr, dg, db, da, dR, dG, dB, dA;
    int                 i;

    Dbg_Assert(origin);
    Dbg_Assert(region);

    if (!root)
        return;

    width = 1 << depth;

    dr = origin->r - region->col1.r;
    dg = origin->g - region->col1.g;
    db = origin->b - region->col1.b;
    da = origin->a - region->col1.a;
    if (dr >= 0 || dg >= 0 || db >= 0 || da >= 0)
    {
        return;
    }

    dR = region->col0.r - origin->r;
    dG = region->col0.g - origin->g;
    dB = region->col0.b - origin->b;
    dA = region->col0.a - origin->a;
    if (dR >= width || dG >= width || dB >= width || dA >= width)
    {
        return;
    }

    /* wholly inside region? */
    if (dr <= -width && dg <= -width && db <= -width && da <= -width)
        if (dR <= 0 && dG <= 0 && dB <= 0 && dA <= 0)
#ifndef CACHEWEIGHTS
            if (depth == 0)    /* we need to visit each leaf */
#endif
            {
                volume->weight += root->Leaf.weight;
                ColorRealAdd(&volume->ac, &volume->ac, &root->Leaf.ac);
                volume->m2 += root->Leaf.m2;
                return;
            }

    /* try children */
    depth--;
    for (i = 0; i < 16; i++)
    {
        Image::RGBA              suborigin;

        suborigin.r = origin->r + (((i >> 3) & 1) << depth);
        suborigin.g = origin->g + (((i >> 2) & 1) << depth);
        suborigin.b = origin->b + (((i >> 1) & 1) << depth);
        suborigin.a = origin->a + (((i >> 0) & 1) << depth);

        addvolume(root->Branch.dir[i], &suborigin, depth, region, volume);
    }    
}

/*************************************************************************/

/* Compute sum over a box of any given statistic */
static LeafNode    *
nVol(LeafNode * Vol, OctNode * root, box * cube)
{
    Image::RGBA              origin;

    Dbg_Assert(root);
    Dbg_Assert(cube);

    origin.r = 0;
    origin.g = 0;
    origin.b = 0;
    origin.a = 0;
    InitLeaf(Vol);
    addvolume(root, &origin, MAXDEPTH, cube, Vol);

    return(Vol);
}

/*************************************************************************/

/* Compute the weighted variance of a box */

/* NB: as with the raw statistics, this is really the variance * size */
static              float
nVar(OctNode * root, box * cube)
{
    LeafNode            Node;

    Dbg_Assert(root);
    Dbg_Assert(cube);

    nVol(&Node, root, cube);

    return(Node.m2 - (ColorRealLengthSq(&Node.ac) / Node.weight));
}

/*************************************************************************/

/* We want to minimize the sum of the variances of two subboxes.
 * The sum(c^2) terms can be ignored since their sum over both subboxes
 * is the same (the sum for the whole box) no matter where we split.
 * The remaining terms have a minus sign in the variance formula,
 * so we drop the minus sign and MAXIMIZE the sum of the two terms.
 */
static              float
nMaximize(OctNode * root, box * cube, int dir, int * cut,
          LeafNode * whole)
{
    box                 infcube;
    LeafNode            left, right;
    float              maxsum, val, lastval;
    int             i;

    Dbg_Assert(root);
    Dbg_Assert(cube);
    Dbg_Assert(cut);
    Dbg_Assert(whole);

    lastval = maxsum = 0.0f;
    *cut = -1;
    infcube = *cube;
    switch (dir)
    {
        case RED:
            for (i = cube->col0.r; i < cube->col1.r; i++)
            {
                infcube.col1.r = (unsigned char) i;

                nVol(&left, root, &infcube);
                ColorRealSub(&right.ac, &whole->ac, &left.ac);
                right.weight = whole->weight - left.weight;
                if ((left.weight > 0.0f) && (right.weight > 0.0f))
                {
                    val = ColorRealLengthSq(&left.ac) / left.weight;
                    val += ColorRealLengthSq(&right.ac) / right.weight;

                    if (val > maxsum)
                    {
                        maxsum = val;
                        *cut = i;
                    }
                    else if (val < lastval)
                    {
                        /* we've past the peak */
                        break;
                    }

                    lastval = val;
                }
            }
            break;

        case GREEN:
            for (i = cube->col0.g; i < cube->col1.g; i++)
            {
                infcube.col1.g = (unsigned char) i;

                nVol(&left, root, &infcube);
                ColorRealSub(&right.ac, &whole->ac, &left.ac);
                right.weight = whole->weight - left.weight;
                if ((left.weight > 0.0f) && (right.weight > 0.0f))
                {
                    val = ColorRealLengthSq(&left.ac) / left.weight;
                    val += ColorRealLengthSq(&right.ac) / right.weight;

                    if (val > maxsum)
                    {
                        maxsum = val;
                        *cut = i;
                    }
                    else if (val < lastval)
                    {
                        /* we've past the peak */
                        break;
                    }

                    lastval = val;
                }
            }
            break;

        case BLUE:
            for (i = cube->col0.b; i < cube->col1.b; i++)
            {
                infcube.col1.b = (unsigned char) i;

                nVol(&left, root, &infcube);
                ColorRealSub(&right.ac, &whole->ac, &left.ac);
                right.weight = whole->weight - left.weight;
                if ((left.weight > 0.0f) && (right.weight > 0.0f))
                {
                    val = ColorRealLengthSq(&left.ac) / left.weight;
                    val += ColorRealLengthSq(&right.ac) / right.weight;

                    if (val > maxsum)
                    {
                        maxsum = val;
                        *cut = i;
                    }
                    else if (val < lastval)
                    {
                        /* we've past the peak */
                        break;
                    }

                    lastval = val;
                }
            }
            break;

        case ALPHA:
            for (i = cube->col0.a; i < cube->col1.a; i++)
            {
                infcube.col1.a = (unsigned char) i;

                nVol(&left, root, &infcube);
                ColorRealSub(&right.ac, &whole->ac, &left.ac);
                right.weight = whole->weight - left.weight;
                if ((left.weight > 0.0f) && (right.weight > 0.0f))
                {
                    val = ColorRealLengthSq(&left.ac) / left.weight;
                    val += ColorRealLengthSq(&right.ac) / right.weight;

                    if (val > maxsum)
                    {
                        maxsum = val;
                        *cut = i;
                    }
                    else if (val < lastval)
                    {
                        /* we've past the peak */
                        break;
                    }

                    lastval = val;
                }
            }
            break;
    }

    return(maxsum);
}

/*************************************************************************/
static              bool
nCut(OctNode * root, box * set1, box * set2)
{
    int             cutr, cutg, cutb, cuta;
    float              maxr, maxg, maxb, maxa;
    LeafNode            whole;

    Dbg_Assert(root);
    Dbg_Assert(set1);
    Dbg_Assert(set2);

    nVol(&whole, root, set1);
    maxr = nMaximize(root, set1, RED, &cutr, &whole);
    maxg = nMaximize(root, set1, GREEN, &cutg, &whole);
    maxb = nMaximize(root, set1, BLUE, &cutb, &whole);
    maxa = nMaximize(root, set1, ALPHA, &cuta, &whole);

    /* did we find any splits? */
    if ( (maxr == 0.0f) &&
         (maxg == 0.0f) &&
         (maxb == 0.0f) &&
         (maxa == 0.0f) )
    {
        return false;
    }

    *set2 = *set1;

    /* NEED TO CHECK FOR ALPHA TOO */

    if (maxr >= maxg)
    {
        if (maxr >= maxb)
        {
            if (maxr >= maxa)
            {
                set1->col1.r = set2->col0.r = (unsigned char) cutr;
            }
            else
            {
                set1->col1.a = set2->col0.a = (unsigned char) cuta;
            }
        }
        else
        {
            if (maxb >= maxa)
            {
                set1->col1.b = set2->col0.b = (unsigned char) cutb;
            }
            else
            {
                set1->col1.a = set2->col0.a = (unsigned char) cuta;
            }
        }
    }
    else if (maxg >= maxb)
    {
        if (maxg >= maxa)
        {
            set1->col1.g = set2->col0.g = (unsigned char) cutg;
        }
        else
        {
            set1->col1.a = set2->col0.a = (unsigned char) cuta;
        }
    }
    else if (maxb >= maxa)
    {
        set1->col1.b = set2->col0.b = (unsigned char) cutb;
    }
    else
    {
        set1->col1.a = set2->col0.a = (unsigned char) cuta;
    }


    return true;
}

/*************************************************************************/
#ifdef CACHEWEIGHTS
static LeafNode    *
CalcNodeWeights(OctNode * root, int depth)
{
    LeafNode           *Leaf;
    int                 i;

    Leaf = NULL;
    if (root)
    {
        Leaf = &root->Leaf;

        /* is it a branch? */
        if (depth > 0)
        {
            InitLeaf(Leaf);
            for (i = 0; i < 16; i++)
            {
                LeafNode           *SubNode =
                    CalcNodeWeights(root->Branch.dir[i], depth - 1);

                if (SubNode)
                {
                    Leaf->weight += SubNode->weight;
                    ColorRealAdd(&Leaf->ac, &Leaf->ac, &SubNode->ac);
                    Leaf->m2 += SubNode->m2;
                }
            }
        }
    }

    return(Leaf);
}
#endif

/*************************************************************************/
static int
CountLeafs(OctNode * root, int depth)
{
    int                 i, n;

    n = 0;
    if (root)
    {
        /* is it a branch? */
        if (depth > 0)
        {
            for (i = 0; i < 16; i++)
            {
                n += CountLeafs(root->Branch.dir[i], depth - 1);
            }
        }
        else
        {
            n = 1;
        }
    }

    return(n);
}

/*************************************************************************/
static int
CountNodes(OctNode * root, int depth)
{
    int                 i, n;

    n = 0;
    if (root)
    {
        n = 1;

        /* is it a branch? */
        if (depth > 0)
        {
            for (i = 0; i < 16; i++)
            {
                n += CountNodes(root->Branch.dir[i], depth - 1);
            }
        }
    }

    return(n);
}

/*************************************************************************/

/* 
 * Note the use of 255.9999f value when generating the scale variable.
 * This is used instead of 255.0f to prevent rounding errors.
 */

#define node2pal(rgb, node)                                              \
{                                                                        \
    int             quantize;                                        \
    float              scale = ( ((node)->weight > 0) ?                 \
                                  (255.9999f / (node)->weight) :         \
                                  (float) 0 );                          \
                                                                         \
    quantize = (int) ((node)->ac.red * scale);                       \
    (rgb)->r = (unsigned char) quantize;                                     \
    quantize = (int) ((node)->ac.green * scale);                     \
    (rgb)->g = (unsigned char) quantize;                                   \
    quantize = (int) ((node)->ac.blue * scale);                      \
    (rgb)->b = (unsigned char) quantize;                                    \
    quantize = (int) ((node)->ac.alpha * scale);                     \
    (rgb)->a = (unsigned char) quantize;                                   \
}                                                                        \

/*************************************************************************/
int
PalQuantResolvePalette(Image::RGBA * palette, int maxcols, PalQuant * pq)
{
    int             numcols, uniquecols, i, k, next;
    
#if (defined(RWDEBUG))
    if (256 < maxcols)
    {
        RWMESSAGE(("256 < %d == maxcols", (int) maxcols));
    }
#endif /* (defined(RWDEBUG)) */


    Dbg_Assert(palette);
    Dbg_Assert(maxcols <= 256);
    Dbg_Assert(pq);

    numcols = maxcols;
    uniquecols = CountLeafs(pq->root, MAXDEPTH);
    if (numcols > uniquecols)
    {
        numcols = uniquecols;
    }

#ifdef CACHEWEIGHTS
    /* cache weightings at every node */
    CalcNodeWeights(pq->root, MAXDEPTH);
#endif

    /* divide and conquer */
    pq->Mcube[0].col0.r = 0;
    pq->Mcube[0].col0.g = 0;
    pq->Mcube[0].col0.b = 0;
    pq->Mcube[0].col0.a = 0;
    pq->Mcube[0].col1.r = 1 << MAXDEPTH;
    pq->Mcube[0].col1.g = 1 << MAXDEPTH;
    pq->Mcube[0].col1.b = 1 << MAXDEPTH;
    pq->Mcube[0].col1.a = 1 << MAXDEPTH;
    pq->Mvv[0] = nVar(pq->root, &pq->Mcube[0]);
    for (i = 1; i < numcols; i++)
    {
        float               maxvar;

        /* find best box to split */
        next = -1;
        maxvar = 0.0f;
        for (k = 0; k < i; k++)
        {
            if (pq->Mvv[k] > maxvar)
            {
                maxvar = pq->Mvv[k];
                next = k;
            }
        }

        /* stop if we couldn't find a box to split */
        if (next == -1)
        {
            break;
        }

        /* split box */
        if (nCut(pq->root, &pq->Mcube[next], &pq->Mcube[i]))
        {
            /* volume test ensures we won't try to cut one-cell box */
            pq->Mvv[next] = nVar(pq->root, &pq->Mcube[next]);
            pq->Mvv[i] = nVar(pq->root, &pq->Mcube[i]);
        }
        else
        {
            /* don't try to split this box again */
            pq->Mvv[next] = 0.0f;
            i--;
        }
    }

    /* extract the new palette */
    for (k = 0; k < maxcols; k++)
    {
        if (k < numcols)
        {
            LeafNode            Node;

            nAssign(k, pq->root, &pq->Mcube[k]);
            nVol(&Node, pq->root, &pq->Mcube[k]);
            node2pal(&palette[k], &Node);
        }
        else
        {
            palette[k].r = 0;
            palette[k].g = 0;
            palette[k].b = 0;
            palette[k].a = 0;
        }
    }

    return(numcols);
}

#if 0
/*************************************************************************/
static              unsigned char
GetIndex(OctNode * root, OctantMap Octs, int depth)
{
    unsigned char             result;

    Dbg_Assert(root);

    if (depth == 0)
    {
        result = root->Leaf.palIndex;
    }
    else
    {
        result = GetIndex(root->Branch.dir[Octs & 15], Octs >> 4, depth - 1);
    }

    return(result);
}
#endif

/*************************************************************************/
bool
PalQuantInit(PalQuant * pq)
{
    int                 i, j, maxval;

    Dbg_Assert(pq);
    Dbg_Assert(!p_splice);

	p_splice = (OctantMap *) Mem::Malloc(sizeof(OctantMap) * MAXCOLOR);

    /* lookup mapping (8) bit-patterns to every 4th bit b31->b00 (least to most) */
    maxval = 1 << MAXDEPTH;
    for (i = 0; i < maxval; i++)
    {
        OctantMap           mask = 0;

        for (j = 0; j < MAXDEPTH; j++)
        {
            mask |= (i & (1 << j)) ? (1 << ((MAXDEPTH - 1 - j) * 4)) : 0;
        }
        p_splice[i] = mask;
    }

    pq->Mcube = (box *) Mem::Calloc(sizeof(box), MAXCOLOR);
    pq->Mvv = (float *) Mem::Calloc(sizeof(float), MAXCOLOR);

    pq->root = CreateCube();
    InitBranch(&pq->root->Branch);

    return true;
}

/*************************************************************************/
static void
DeleteOctTree(PalQuant * pq, OctNode * root, int depth)
{
    int                 i;

    Dbg_Assert(pq);

    if (root)
    {
        /* is it a branch? */
        if (depth > 0)
        {
            for (i = 0; i < 16; i++)
            {
                DeleteOctTree(pq, root->Branch.dir[i], depth - 1);
            }
        }

		Mem::Free(root);
    }
}

/*************************************************************************/
void
PalQuantTerm(PalQuant * pq)
{

    Dbg_Assert(pq);
    Dbg_Assert(p_splice);


    DeleteOctTree(pq, pq->root, MAXDEPTH);
    pq->root = (OctNode *)NULL;

    Mem::Free( pq->Mvv );
    Mem::Free( pq->Mcube );
	Mem::Free( p_splice );
	p_splice = NULL;
}

/*************************************************************************/
void GeneratePalette( Image::RGBA *p_palette, uint8* src_image, int width, int height, int image_bpp, int max_colors )
{
	int num_colors;
	PalQuant pal_quant;

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());

	if( !PalQuantInit( &pal_quant ))
	{
		return;
	}

	PalQuantAddImage( &pal_quant, src_image, width, height, image_bpp, 1 );	
	
	// Create a palette for all of the mips
	num_colors = PalQuantResolvePalette( p_palette, max_colors, &pal_quant );

	// And swizzle for the PS2
	Image::RGBA temp_rgba;
	for (int j=0; j<256; j+=32)
	{
		for (int k=0; k<8; k++)
		{
			temp_rgba = p_palette[j+k+8];
			p_palette[j+k+8] = p_palette[j+k+16];
			p_palette[j+k+16] = temp_rgba;
		}
	}

	
	PalQuantTerm( &pal_quant );

	Mem::Manager::sHandle().PopContext();
}


} // Namespace Nx  			

