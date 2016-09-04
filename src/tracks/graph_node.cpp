//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2009-2015 Joerg Henrichs
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, B

#include "tracks/graph_node.hpp"

#include "io/file_manager.hpp"
#include "io/xml_node.hpp"
#include "matrix4.h"
#include "tracks/quad_graph.hpp"
#include "utils/log.hpp"

// ----------------------------------------------------------------------------
GraphNode::GraphNode(const Vec3 &p0, const Vec3 &p1, const Vec3 &p2,
                     const Vec3 &p3, const Vec3 &normal,
                     unsigned int node_index, bool invisible,
                     bool ai_ignore)
          :Quad(p0, p1, p2, p3)
{
    m_invisible           = invisible;
    m_ai_ignore           = ai_ignore;
    m_normal              = normal;
    m_node_index          = node_index;
    m_distance_from_start = -1.0f;

    // The following values should depend on the actual orientation
    // of the quad. ATM we always assume that indices 0,1 are the lower end,
    // and 2,3 are the upper end (or the reverse if reverse mode is selected).
    // The width is the average width at the beginning and at the end.
    m_right_unit_vector = ( m_p[0]-m_p[1]
                           +m_p[3]-m_p[2]) * 0.5f;
    m_right_unit_vector.normalize();

    m_width = (  (m_p[1]-m_p[0]).length()
               + (m_p[3]-m_p[2]).length() ) * 0.5f;

    if(QuadGraph::get()->isReverse())
    {
        m_lower_center = (m_p[2]+m_p[3]) * 0.5f;
        m_upper_center = (m_p[0]+m_p[1]) * 0.5f;
        m_right_unit_vector *= -1.0f;
    }
    else
    {
        m_lower_center = (m_p[0]+m_p[1]) * 0.5f;
        m_upper_center = (m_p[2]+m_p[3]) * 0.5f;
    }

}   // GraphNode

// ----------------------------------------------------------------------------
/** Adds a successor to a node. This function will also pre-compute certain
 *  values (like distance from this node to the successor, angle (in world)
 *  between this node and the successor.
 *  \param to The index of the graph node of the successor.
 */
void GraphNode::addSuccessor(unsigned int to)
{
    m_successor_nodes.push_back(to);
    // to is the graph node
    GraphNode &gn_to = QuadGraph::get()->getNode(to);

    // Note that the first predecessor is (because of the way the quad graph
    // is exported) the most 'natural' one, i.e. the one on the main
    // driveline.
    gn_to.m_predecessor_nodes.push_back(m_node_index);

    Vec3 d = m_lower_center - gn_to.m_lower_center;
    m_distance_to_next.push_back(d.length());

    Vec3 diff = gn_to.getCenter() - getCenter();

    core::CMatrix4<float> m;
    m.buildRotateFromTo(getNormal().toIrrVector(), 
                        Vec3(0, 1, 0).toIrrVector());
    core::vector3df diffRotated;
    m.rotateVect(diffRotated, diff.toIrrVector());

    m_angle_to_next.push_back(atan2(diffRotated.X, diffRotated.Z));

}   // addSuccessor

// ----------------------------------------------------------------------------
/** If this node has more than one successor, it will set up a vector that
 *  contains the direction to use when a certain graph node X should be
 *  reached.
 */
void GraphNode::setupPathsToNode()
{
    if(m_successor_nodes.size()<2) return;

    const unsigned int num_nodes = QuadGraph::get()->getNumNodes();
    m_path_to_node.resize(num_nodes);

    // Initialise each graph node with -1, indicating that
    // it hasn't been reached yet.
    for(unsigned int i=0; i<num_nodes; i++)
        m_path_to_node[i] = -1;

    // Indicate that this node can be reached from this node by following
    // successor 0 - just a dummy value that might only be used during the
    // recursion below.
    m_path_to_node[m_node_index] = 0;

    // A simple depth first search is used to determine which successor to
    // use to reach a certain graph node. Using Dijkstra's algorithm  would
    // give the shortest way to reach a certain node, but the shortest way
    // might involve some shortcuts which are hidden, and should therefore
    // not be used.
    for(unsigned int i=0; i<getNumberOfSuccessors(); i++)
    {
        GraphNode &gn = QuadGraph::get()->getNode(getSuccessor(i));
        gn.markAllSuccessorsToUse(i, &m_path_to_node);
    }
#ifdef DEBUG
    for(unsigned int i = 0; i < m_path_to_node.size(); ++i)
    {
        if(m_path_to_node[i] == -1)
            Log::warn("GraphNode", "No path to node %d found on graph node %d.",
                   i, m_node_index);
    }
#endif
}   // setupPathsToNode

// ----------------------------------------------------------------------------
/** This function marks that the successor n should be used to reach this
 *  node. It then recursively (depth first) does the same for all its
 *  successors. Depth-first
 *  \param n The successor which should be used in m_path_node to reach
 *         this node.
 *  \param path_to_node The path-to-node data structure of the node for
 *         which the paths are currently determined.
 */
void GraphNode::markAllSuccessorsToUse(unsigned int n,
                                       PathToNodeVector *path_to_node)
{
    // End recursion if the path to this node has already been found.
    if( (*path_to_node)[m_node_index] >-1) return;

    (*path_to_node)[m_node_index] = n;
    for(unsigned int i=0; i<getNumberOfSuccessors(); i++)
    {
        GraphNode &gn = QuadGraph::get()->getNode(getSuccessor(i));
        gn.markAllSuccessorsToUse(n, path_to_node);
    }
}   // markAllSuccesorsToUse

// ----------------------------------------------------------------------------
void GraphNode::setDirectionData(unsigned int successor, DirectionType dir,
                                 unsigned int last_node_index)
{
    if(m_direction.size()<successor+1)
    {
        m_direction.resize(successor+1);
        m_last_index_same_direction.resize(successor+1);
    }
    m_direction[successor]                 = dir;
    m_last_index_same_direction[successor] = last_node_index;
}   // setDirectionData

// ----------------------------------------------------------------------------
void GraphNode::setChecklineRequirements(int latest_checkline)
{
    m_checkline_requirements.push_back(latest_checkline);
}   // setChecklineRequirements

// ----------------------------------------------------------------------------
/** Returns true if the index-successor of this node is one that the AI
 *  is allowed to use.
 *  \param index Index of the successor.
 */
bool GraphNode::ignoreSuccessorForAI(unsigned int i) const
{
    return QuadGraph::get()->getNode(m_successor_nodes[i]).letAIIgnore();
}   // ignoreSuccessorForAI
