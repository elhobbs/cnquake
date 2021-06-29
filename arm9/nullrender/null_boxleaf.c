#include "quakedef.h"

#if 0
/*
=============
CM_BoxLeafnums

Fills in a list of all the leafs touched
=============
*/
int		leaf_count, leaf_maxcount;
mleaf_t		**leaf_list;
float	*leaf_mins, *leaf_maxs;
mnode_t *leaf_topnode;
bmodel_t *leaf_bmodel;

void CM_BoxLeafnums_r (mnode_t *node)
{
	mplane_t	*plane;
	int		s;

	while (1)
	{
		if (node->contents < 0)
		{
			if (leaf_count >= leaf_maxcount)
			{
//				Com_Printf ("CM_BoxLeafnums_r: overflow\n");
				return;
			}
			leaf_list[leaf_count++] = (mleaf_t *)node;
			return;
		}
	
		plane = node->plane;
//		s = BoxOnPlaneSide (leaf_mins, leaf_maxs, plane);
		s = BOX_ON_PLANE_SIDE(leaf_mins, leaf_maxs, plane);
		if (s == 1)
			node = node->children[0];
		else if (s == 2)
			node = node->children[1];
		else
		{	// go down both
			if (leaf_topnode == 0)
				leaf_topnode = node;
			CM_BoxLeafnums_r (node->children[0]);
			node = node->children[1];
		}

	}
}

int	CM_BoxLeafnums_headnode (vec3_t mins, vec3_t maxs, mleaf_t **list, int listsize, mnode_t* headnode, mnode_t **topnode)
{
	leaf_list = list;
	leaf_count = 0;
	leaf_maxcount = listsize;
	leaf_mins = mins;
	leaf_maxs = maxs;

	leaf_topnode = 0;

	CM_BoxLeafnums_r (headnode);

	if (topnode)
		*topnode = leaf_topnode;

	return leaf_count;
}

int	CM_BoxLeafnums (vec3_t mins, vec3_t maxs, mleaf_t **list, int listsize, mnode_t **topnode)
{
	leaf_bmodel = (bmodel_t*)sv.worldmodel->cache.data;
	return CM_BoxLeafnums_headnode (mins, maxs, list,
		listsize,leaf_bmodel->nodes, topnode);
}
#endif
