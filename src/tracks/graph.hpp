//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2016 SuperTuxKart Team
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
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifndef HEADER_GRAPH_HPP
#define HEADER_GRAPH_HPP

#include "utils/no_copy.hpp"
#include "utils/vec3.hpp"

#include <dimension2d.h>

#include <string>
#include <vector>

namespace irr
{
    namespace scene { class ISceneNode; class IMesh; class IMeshBuffer; }
    namespace video { class ITexture; struct S3DVertex; class SColor; }
}

using namespace irr;

class FrameBuffer;
class Quad;
class RTT;

/**
 *  \ingroup tracks
 */
class Graph : public NoCopy
{
protected:
    static Graph* m_graph;

    std::vector<Quad*> m_all_nodes;

    /** The 2d bounding box, used for hashing. */
    Vec3 m_bb_min;

    Vec3 m_bb_max;

    // ------------------------------------------------------------------------
    /** Factory method to dynamic create 2d / 3d quad. */
    void createQuad(const Vec3 &p0, const Vec3 &p1, const Vec3 &p2,
                    const Vec3 &p3, unsigned int node_index,
                    bool invisible, bool ai_ignore, bool is_arena);

private:
    RTT* m_new_rtt;

    /** The node of the graph mesh. */
    scene::ISceneNode *m_node;

    /** The mesh of the graph mesh. */
    scene::IMesh *m_mesh;

    /** The actual mesh buffer storing the graph. */
    scene::IMeshBuffer *m_mesh_buffer;

    /** Scaling for mini map. */
    float m_scaling;

    // ------------------------------------------------------------------------
    void createMesh(bool show_invisible=true,
                    bool enable_transparency=false,
                    const video::SColor *track_color=NULL);
    // ------------------------------------------------------------------------
    void cleanupDebugMesh();
    // ------------------------------------------------------------------------
    virtual bool hasLapLine() const = 0;
    // ------------------------------------------------------------------------
    virtual void differentNodeColor(int n, video::SColor* c) const = 0;

public:
    static const int UNKNOWN_SECTOR;
    // ------------------------------------------------------------------------
    /** Returns the one instance of this object. It is possible that there
     *  is no instance created (e.g. battle mode without navmesh) so we don't
     *  assert that an instance exist. */
    static Graph* get() { return m_graph; }
    // ------------------------------------------------------------------------
    /** Set the graph (either driveline or arena graph for now). */
    static void setGraph(Graph* graph)
    {
        assert(m_graph == NULL);
        m_graph = graph;
    }   // create
    // ------------------------------------------------------------------------
    /** Cleans up the graph. It is possible that this function is called even
     *  if no instance exists (e.g. in battle mode without navmesh). So it is
     *  not an error if there is no instance. */
    static void destroy()
    {
        if (m_graph)
        {
            delete m_graph;
            m_graph = NULL;
        }
    }   // destroy
    // ------------------------------------------------------------------------
    Graph();
    // ------------------------------------------------------------------------
    virtual ~Graph();
    // ------------------------------------------------------------------------
    void createDebugMesh();
    // ------------------------------------------------------------------------
    void makeMiniMap(const core::dimension2du &where, const std::string &name,
                     const video::SColor &fill_color,
                     video::ITexture** oldRttMinimap,
                     FrameBuffer** newRttMinimap);
    // ------------------------------------------------------------------------
    void mapPoint2MiniMap(const Vec3 &xyz, Vec3 *out) const;
    // ------------------------------------------------------------------------
    Quad* getQuad(unsigned int i) const
    {
        assert(i < m_all_nodes.size());
        return m_all_nodes[i];
    }
    // ------------------------------------------------------------------------
    unsigned int getNumNodes() const             { return m_all_nodes.size(); }
    // ------------------------------------------------------------------------
    void findRoadSector(const Vec3& XYZ, int *sector,
                        std::vector<int> *all_sectors = NULL,
                        bool ignore_vertical = false) const;
    // ------------------------------------------------------------------------
    int findOutOfRoadSector(const Vec3& xyz,
                            const int curr_sector = UNKNOWN_SECTOR,
                            std::vector<int> *all_sectors = NULL,
                            bool ignore_vertical = false) const;

};   // Graph

#endif
