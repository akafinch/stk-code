//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2014-2015 SuperTuxKart-Team
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

#include "2dutils.hpp"

#include "graphics/central_settings.hpp"
#include "graphics/shaders.hpp"
#include "glwrap.hpp"
#include "utils/cpp2011.hpp"

#include "../../lib/irrlicht/source/Irrlicht/COpenGLTexture.h"

// ============================================================================
class Primitive2DList : public Shader<Primitive2DList>,
                        public TextureReadNew<ST_BILINEAR_FILTERED >
{
public:
    Primitive2DList()
    {
        loadProgram(OBJECT, GL_VERTEX_SHADER, "primitive2dlist.vert",
                            GL_FRAGMENT_SHADER, "transparent.frag");
        assignUniforms();
        assignSamplerNames(m_program, 0, "tex", ST_BILINEAR_FILTERED);
    }   // Primitive2DList
};   //Primitive2DList

// ============================================================================
static void drawTexColoredQuad(const video::ITexture *texture,
                               const video::SColor *col, float width,
                               float height, float center_pos_x,
                               float center_pos_y, float tex_center_pos_x,
                               float tex_center_pos_y, float tex_width,
                               float tex_height)
{
    unsigned colors[] = {
        col[0].getRed(), col[0].getGreen(), col[0].getBlue(), col[0].getAlpha(),
        col[1].getRed(), col[1].getGreen(), col[1].getBlue(), col[1].getAlpha(),
        col[2].getRed(), col[2].getGreen(), col[2].getBlue(), col[2].getAlpha(),
        col[3].getRed(), col[3].getGreen(), col[3].getBlue(), col[3].getAlpha(),
    };

    glBindBuffer(GL_ARRAY_BUFFER, UIShader::ColoredTextureRectShader::getInstance()->colorvbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 16 * sizeof(unsigned), colors);

    UIShader::ColoredTextureRectShader::getInstance()->use();
    glBindVertexArray(UIShader::ColoredTextureRectShader::getInstance()->vao);

    UIShader::ColoredTextureRectShader::getInstance()->setTextureUnits(static_cast<const irr::video::COpenGLTexture*>(texture)->getOpenGLTextureName());
    UIShader::ColoredTextureRectShader::getInstance()->setUniforms(
        core::vector2df(center_pos_x, center_pos_y), core::vector2df(width, height),
        core::vector2df(tex_center_pos_x, tex_center_pos_y), core::vector2df(tex_width, tex_height));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGetError();
}   // drawTexColoredQuad

// ----------------------------------------------------------------------------
static void drawTexQuad(GLuint texture, float width, float height,
                        float center_pos_x, float center_pos_y, 
                        float tex_center_pos_x, float tex_center_pos_y,
                        float tex_width, float tex_height)
{
    UIShader::TextureRectShader::getInstance()->use();
    glBindVertexArray(SharedObject::UIVAO);

    UIShader::TextureRectShader::getInstance()->setTextureUnits(texture);
    UIShader::TextureRectShader::getInstance()->setUniforms(
                    core::vector2df(center_pos_x, center_pos_y), 
                    core::vector2df(width, height),
                    core::vector2df(tex_center_pos_x, tex_center_pos_y),
                    core::vector2df(tex_width, tex_height)                );

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGetError();
}   // drawTexQuad


// ----------------------------------------------------------------------------
static void getSize(unsigned texture_width, unsigned texture_height,
                    bool textureisRTT, const core::rect<s32>& destRect,
                    const core::rect<s32>& sourceRect,
                    float &width, float &height,
                    float &center_pos_x, float &center_pos_y,
                    float &tex_width, float &tex_height,
                    float &tex_center_pos_x, float &tex_center_pos_y   )
{
    core::dimension2d<u32> frame_size = irr_driver->getActualScreenSize();
    const int screen_w = frame_size.Width;
    const int screen_h =  frame_size.Height;
    center_pos_x = float(destRect.UpperLeftCorner.X + destRect.LowerRightCorner.X);
    center_pos_x /= screen_w;
    center_pos_x -= 1.;
    center_pos_y = float(destRect.UpperLeftCorner.Y + destRect.LowerRightCorner.Y);
    center_pos_y /= screen_h;
    center_pos_y = float(1.f - center_pos_y);
    width = float(destRect.LowerRightCorner.X - destRect.UpperLeftCorner.X);
    width /= screen_w;
    height = float(destRect.LowerRightCorner.Y - destRect.UpperLeftCorner.Y);
    height /= screen_h;

    tex_center_pos_x = float(sourceRect.UpperLeftCorner.X + sourceRect.LowerRightCorner.X);
    tex_center_pos_x /= texture_width * 2.f;
    tex_center_pos_y = float(sourceRect.UpperLeftCorner.Y + sourceRect.LowerRightCorner.Y);
    tex_center_pos_y /= texture_height * 2.f;
    tex_width = float(sourceRect.LowerRightCorner.X - sourceRect.UpperLeftCorner.X);
    tex_width /= texture_width * 2.f;
    tex_height = float(sourceRect.LowerRightCorner.Y - sourceRect.UpperLeftCorner.Y);
    tex_height /= texture_height * 2.f;

    if (textureisRTT)
        tex_height = -tex_height;

    const f32 invW = 1.f / static_cast<f32>(texture_width);
    const f32 invH = 1.f / static_cast<f32>(texture_height);
    const core::rect<f32> tcoords(sourceRect.UpperLeftCorner.X  * invW,
                                  sourceRect.UpperLeftCorner.Y  * invH,
                                  sourceRect.LowerRightCorner.X * invW,
                                  sourceRect.LowerRightCorner.Y * invH);
}   // getSize

// ----------------------------------------------------------------------------
void draw2DImage(const video::ITexture* texture, 
                 const core::rect<s32>& destRect,
                 const core::rect<s32>& sourceRect,
                 const core::rect<s32>* clip_rect,
                 const video::SColor &colors, bool use_alpha_channel_of_texture)
{
    if (!CVS->isGLSL())
    {
        video::SColor duplicatedArray[4] = { colors, colors, colors, colors };
        draw2DImage(texture, destRect, sourceRect, clip_rect, duplicatedArray,
                    use_alpha_channel_of_texture);
        return;
    }

    float width, height, center_pos_x, center_pos_y;
    float tex_width, tex_height, tex_center_pos_x, tex_center_pos_y;

    getSize(texture->getSize().Width, texture->getSize().Height, 
            texture->isRenderTarget(), destRect, sourceRect, width, height,
            center_pos_x, center_pos_y, tex_width, tex_height,
            tex_center_pos_x, tex_center_pos_y);

    if (use_alpha_channel_of_texture)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        glDisable(GL_BLEND);
    }

    if (clip_rect)
    {
        if (!clip_rect->isValid())
            return;

        glEnable(GL_SCISSOR_TEST);
        const core::dimension2d<u32>& render_target_size = 
                           irr_driver->getActualScreenSize();
        glScissor(clip_rect->UpperLeftCorner.X, 
                  render_target_size.Height - clip_rect->LowerRightCorner.Y,
                  clip_rect->getWidth(), clip_rect->getHeight());
    }

    UIShader::UniformColoredTextureRectShader::getInstance()->use();
    glBindVertexArray(SharedObject::UIVAO);

    const video::COpenGLTexture *c_texture = 
        static_cast<const video::COpenGLTexture*>(texture);
    UIShader::UniformColoredTextureRectShader::getInstance()
        ->setTextureUnits(c_texture->getOpenGLTextureName());

    UIShader::UniformColoredTextureRectShader::getInstance()
        ->setUniforms(core::vector2df(center_pos_x, center_pos_y),
                      core::vector2df(width, height),
                      core::vector2df(tex_center_pos_x, tex_center_pos_y),
                      core::vector2df(tex_width, tex_height),
                      colors);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (clip_rect)
        glDisable(GL_SCISSOR_TEST);
    glUseProgram(0);

    glGetError();
}   // draw2DImage

// ----------------------------------------------------------------------------
void draw2DImageFromRTT(GLuint texture, size_t texture_w, size_t texture_h,
                        const core::rect<s32>& destRect,
                        const core::rect<s32>& sourceRect,
                        const core::rect<s32>* clip_rect,
                        const video::SColor &colors,
                        bool use_alpha_channel_of_texture)
{
    if (use_alpha_channel_of_texture)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    float width, height,
        center_pos_x, center_pos_y,
        tex_width, tex_height,
        tex_center_pos_x, tex_center_pos_y;

    getSize((int)texture_w, (int)texture_h, true,
        destRect, sourceRect, width, height, center_pos_x, center_pos_y,
        tex_width, tex_height, tex_center_pos_x, tex_center_pos_y);

    UIShader::UniformColoredTextureRectShader::getInstance()->use();
    glBindVertexArray(SharedObject::UIVAO);

    UIShader::UniformColoredTextureRectShader::getInstance()->setTextureUnits(texture);
    UIShader::UniformColoredTextureRectShader::getInstance()
        ->setUniforms(core::vector2df(center_pos_x, center_pos_y), 
                      core::vector2df(width, height),
                      core::vector2df(tex_center_pos_x, tex_center_pos_y),
                      core::vector2df(tex_width, tex_height), colors        );

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}   // draw2DImageFromRTT

// ----------------------------------------------------------------------------
void draw2DImage(const video::ITexture* texture, 
                 const core::rect<s32>& destRect,
                 const core::rect<s32>& sourceRect,
                 const core::rect<s32>* clip_rect,
                 const video::SColor* const colors,
                 bool use_alpha_channel_of_texture)
{
    if (!CVS->isGLSL())
    {
        irr_driver->getVideoDriver()->draw2DImage(texture, destRect, sourceRect,
                                                  clip_rect, colors,
                                                  use_alpha_channel_of_texture);
        return;
    }

    float width, height, center_pos_x, center_pos_y, tex_width, tex_height;
    float tex_center_pos_x, tex_center_pos_y;

    getSize(texture->getSize().Width, texture->getSize().Height,
            texture->isRenderTarget(), destRect, sourceRect, width, height, 
            center_pos_x, center_pos_y, tex_width, tex_height,
            tex_center_pos_x, tex_center_pos_y);

    if (use_alpha_channel_of_texture)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        glDisable(GL_BLEND);
    }
    if (clip_rect)
    {
        if (!clip_rect->isValid())
            return;

        glEnable(GL_SCISSOR_TEST);
        const core::dimension2d<u32>& render_target_size =
                            irr_driver->getActualScreenSize();
        glScissor(clip_rect->UpperLeftCorner.X, 
                  render_target_size.Height - clip_rect->LowerRightCorner.Y,
                  clip_rect->getWidth(), clip_rect->getHeight());
    }
    if (colors)
    {
        drawTexColoredQuad(texture, colors, width, height, center_pos_x,
                           center_pos_y, tex_center_pos_x, tex_center_pos_y,
                           tex_width, tex_height);
    }
    else
    {
        const video::COpenGLTexture *c_texture = 
                            static_cast<const video::COpenGLTexture*>(texture);
        drawTexQuad(c_texture->getOpenGLTextureName(), width, height,
                    center_pos_x, center_pos_y, tex_center_pos_x,
                    tex_center_pos_y, tex_width, tex_height);
    }
    if (clip_rect)
        glDisable(GL_SCISSOR_TEST);
    glUseProgram(0);

    glGetError();
}   // draw2DImage

// ----------------------------------------------------------------------------
void draw2DVertexPrimitiveList(video::ITexture *tex, const void* vertices,
                               u32 vertexCount, const void* indexList, 
                               u32 primitiveCount, video::E_VERTEX_TYPE vType,
                               scene::E_PRIMITIVE_TYPE pType,
                               video::E_INDEX_TYPE iType)
{
    if (!CVS->isGLSL())
    {
        irr_driver->getVideoDriver()
            ->draw2DVertexPrimitiveList(vertices, vertexCount, indexList,
                                        primitiveCount, vType, pType, iType);
        return;
    }

    GLuint tmpvao, tmpvbo, tmpibo;
    primitiveCount += 2;
    glGenVertexArrays(1, &tmpvao);
    glBindVertexArray(tmpvao);
    glGenBuffers(1, &tmpvbo);
    glBindBuffer(GL_ARRAY_BUFFER, tmpvbo);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * getVertexPitchFromType(vType),
                 vertices, GL_STREAM_DRAW);
    glGenBuffers(1, &tmpibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tmpibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, primitiveCount * sizeof(u16),
                 indexList, GL_STREAM_DRAW);

    VertexUtils::bindVertexArrayAttrib(vType);

    Primitive2DList::getInstance()->use();
    Primitive2DList::getInstance()->setUniforms();
    const video::SOverrideMaterial &m = irr_driver->getVideoDriver()
                                                  ->getOverrideMaterial();
    compressTexture(tex, false);
    Primitive2DList::getInstance()->setTextureUnits(getTextureGLuint(tex));
    glDrawElements(GL_TRIANGLE_FAN, primitiveCount, GL_UNSIGNED_SHORT, 0);

    glDeleteVertexArrays(1, &tmpvao);
    glDeleteBuffers(1, &tmpvbo);
    glDeleteBuffers(1, &tmpibo);

}   // draw2DVertexPrimitiveList

// ----------------------------------------------------------------------------
void GL32_draw2DRectangle(video::SColor color, const core::rect<s32>& position,
                          const core::rect<s32>* clip)
{

    if (!CVS->isGLSL())
    {
        irr_driver->getVideoDriver()->draw2DRectangle(color, position, clip);
        return;
    }

    core::dimension2d<u32> frame_size = irr_driver->getActualScreenSize();
    const int screen_w = frame_size.Width;
    const int screen_h = frame_size.Height;
    float center_pos_x = float(position.UpperLeftCorner.X + position.LowerRightCorner.X);
    center_pos_x /= screen_w;
    center_pos_x -= 1;
    float center_pos_y = float(position.UpperLeftCorner.Y + position.LowerRightCorner.Y);
    center_pos_y /= screen_h;
    center_pos_y = 1 - center_pos_y;
    float width = float(position.LowerRightCorner.X - position.UpperLeftCorner.X);
    width /= screen_w;
    float height = float(position.LowerRightCorner.Y - position.UpperLeftCorner.Y);
    height /= screen_h;

    if (color.getAlpha() < 255)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        glDisable(GL_BLEND);
    }

    if (clip)
    {
        if (!clip->isValid())
            return;

        glEnable(GL_SCISSOR_TEST);
        const core::dimension2d<u32>& render_target_size = 
                                            irr_driver->getActualScreenSize();
        glScissor(clip->UpperLeftCorner.X,
                  render_target_size.Height - clip->LowerRightCorner.Y,
                  clip->getWidth(), clip->getHeight());
    }

    UIShader::ColoredRectShader::getInstance()->use();
    glBindVertexArray(SharedObject::UIVAO);
    UIShader::ColoredRectShader::getInstance()
        ->setUniforms(core::vector2df(center_pos_x, center_pos_y),
                      core::vector2df(width, height), color        );

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    if (clip)
        glDisable(GL_SCISSOR_TEST);
    glUseProgram(0);

    glGetError();
}   // GL32_draw2DRectangle
