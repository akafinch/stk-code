//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2015 SuperTuxKart-Team
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

#ifndef HEADER_FIXED_PIPELINE_RENDERER_HPP
#define HEADER_FIXED_PIPELINE_RENDERER_HPP

#include "graphics/abstract_renderer.hpp"
#include <map>

class RenderTarget;
class GL1RenderTarget;

class FixedPipelineRenderer: public AbstractRenderer
{  
public:
    
    void onLoadWorld()  ;
    void onUnloadWorld();
    
    void render(float dt);
    void renderScene(irr::scene::ICameraSceneNode * const camnode,
                     float dt, bool hasShadows, bool forceRTT){}
    void updateLightsInfo(irr::scene::ICameraSceneNode * const camnode,
                              float dt){}
    
    std::unique_ptr<RenderTarget> createRenderTarget(const irr::core::dimension2du &dimension,
                                                     const std::string &name);
    
};

#endif //HEADER_FIXED_PIPELINE_RENDERER_HPP
