//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2017 SuperTuxKart-Team
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

#include "graphics/stk_tex_manager.hpp"
#include "graphics/central_settings.hpp"
#include "graphics/irr_driver.hpp"
#include "graphics/stk_texture.hpp"
#include "io/file_manager.hpp"
#include "utils/string_utils.hpp"
#include "utils/log.hpp"

// ----------------------------------------------------------------------------
STKTexManager::~STKTexManager()
{
}   // ~STKTexManager

// ----------------------------------------------------------------------------
STKTexture* STKTexManager::findTextureInFileSystem(const std::string& filename,
                                                   std::string* full_path)
{
    *full_path = file_manager->getFileSystem()->getAbsolutePath
        (file_manager->searchTexture(filename).c_str()).c_str();
    if (full_path->empty())
    {
        Log::warn("STKTexManager", "Failed to load %s.", filename.c_str());
        return NULL;
    }
    for (auto p : m_all_textures)
    {
        if (p.second == NULL)
            continue;
        if (*full_path == p.first)
            return p.second;
    }

    return NULL;
}   // findTextureInFileSystem

// ----------------------------------------------------------------------------
STKTexture* STKTexManager::findTexturePathless(const std::string& filename)
{
    for (auto p : m_all_textures)
    {
        if (p.second == NULL)
            continue;
        std::string lc_name = StringUtils::toLowerCase(filename);
        std::string lc_path =
            StringUtils::toLowerCase(p.first);
        std::string tex_name = StringUtils::getBasename(lc_path);
        if (lc_name == tex_name || lc_name == lc_path)
            return p.second;
    }

    return NULL;
}   // findTexturePathless

// ----------------------------------------------------------------------------
STKTexture* STKTexManager::getTexture(const std::string& path, bool srgb,
                                      bool premul_alpha, bool set_material,
                                      bool mesh_tex, bool no_upload)
{
    auto ret = m_all_textures.find(path);
    if (!no_upload && ret != m_all_textures.end())
        return ret->second;

    STKTexture* new_texture = NULL;
    std::string full_path;
    if (path.find('/') == std::string::npos)
    {
        new_texture = findTextureInFileSystem(path, &full_path);
        if (full_path.empty())
            return NULL;
        if (!no_upload && new_texture)
            return new_texture;
    }

    new_texture = new STKTexture(full_path.empty() ? path : full_path, srgb,
        premul_alpha, set_material, mesh_tex, no_upload);
    if (new_texture->getOpenGLTextureName() == 0 && !no_upload)
    {
        m_all_textures[new_texture->getName().getPtr()] = NULL;
        delete new_texture;
        return NULL;
    }

    if (!no_upload)
        addTexture(new_texture);
    return new_texture;
}   // getTexture

// ----------------------------------------------------------------------------
void STKTexManager::addTexture(STKTexture* t)
{
    m_all_textures[t->getName().getPtr()] = t;
}   // addTexture

// ----------------------------------------------------------------------------
void STKTexManager::dumpAllTexture(bool mesh_texture)
{
    for (auto p : m_all_textures)
    {
        if (!p.second || (mesh_texture && !p.second->isMeshTexture()))
            continue;
        Log::info("STKTexManager", "%s loc: %p", p.first.c_str(), p.second);
    }
}   // dumpAllTexture

// ----------------------------------------------------------------------------
int STKTexManager::dumpTextureUsage()
{
    int size = 0;
    for (auto p : m_all_textures)
    {
        if (p.second == NULL)
            continue;
        size += p.second->getTextureSize() / 1024 / 1024;
    }
    Log::info("STKTexManager", "Total %dMB", size);
    return size;
}   // dumpAllTexture

// ----------------------------------------------------------------------------
STKTexture* STKTexManager::getUnicolorTexture(const irr::video::SColor &c)
{
    std::string name = StringUtils::toString(c.color) + "unic";
    auto ret = m_all_textures.find(name);
    if (ret != m_all_textures.end())
        return ret->second;

    uint32_t color[4] = { c.color, c.color, c.color, c.color };
    video::IImage* image = irr_driver->getVideoDriver()
        ->createImageFromData(video::ECF_A8R8G8B8,
        core::dimension2d<u32>(2, 2), color);
    STKTexture* texture = new STKTexture(image, name);
    addTexture(texture);
    return texture;

}   // getUnicolorTexture
