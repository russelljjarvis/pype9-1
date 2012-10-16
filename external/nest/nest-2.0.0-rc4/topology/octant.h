#ifndef OCTANT_H
#define OCTANT_H

/*
 *  octant.h
 *
 *  This file is part of NEST
 *
 *  Copyright (C) 2008 by
 *  The NEST Initiative
 *
 *  See the file AUTHORS for details.
 *
 *  Permission is granted to compile and modify
 *  this file for non-commercial use.
 *  See the file LICENSE for details.
 *
 */

/*
    This file is part of the NEST topology module.
    Author: Kittel Austvoll
*/

#include <vector>
#include "nest.h"
#include "node.h"

#include "position.h"

#include "region.h"
#include "quadrant.h"

namespace nest
{
  template <class DataType>
    class Position;

  class Region;
  class NodeWrapper;

  /**
   * A Octant object represents a node (branch or leaf) in a 
   * Quadtree structure. Any octant covers a specific region 
   * in a 2D space. A leaf octant contains a list of Node
   * pointers and their corresponding positions. A branch 
   * octant contains a list of four other octants, each 
   * covering a region corresponding to the upper-left, 
   * lower-left, upper-right and lower-left corner of their mother
   * octant.
   *
   */
  class Octant: public Quadrant
  {
  public:
    Octant();
    
    /**
     * Create a Octant that covers the region defined by the two 
     * input positions.
     * @param lower_left  Upper left corner of octant.
     * @param upper_right Lower right corner of octant.
     */
    Octant(const Position<double_t>& pos, 
	   const Position<double_t>&,
	   index max_nodes);

    ~Octant();

    /**
     * Transfer a leaf octant to a regular octant with eight 
     * children regions.
     */
    void split();

    /**
     * Find the nodes in the octant that are within the circular 
     * region defined by the input center and radius.
     * @param nodes  Vector where the output nodes are stored.
     * @param center Center of circular region.
     * @param radius Radius of circular region.
     * @param extent layer extent for periodic boundary conditions, or 0 otherwise
     */
    void get_nodes(lockPTR<std::vector<NodeWrapper> > nodes,
		   Volume const * const box,
		   std::vector<double_t> * extent=0);

    void find_region(const Position<double_t>& upper_left, 
		     Quadrant* lower_right, 
		     std::list<Octant>& quad_region, 
		     bool within_region);


  private:

    // Octant children (empty if octant is a leaf)
    std::vector<Octant*> children_;

  };

}

#endif
