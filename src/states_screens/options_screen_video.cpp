//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2009-2015 Marianne Gagnon
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

#include "states_screens/options_screen_video.hpp"

#include "audio/sfx_manager.hpp"
#include "audio/sfx_base.hpp"
#include "config/user_config.hpp"
#include "graphics/central_settings.hpp"
#include "graphics/irr_driver.hpp"
#include "graphics/shared_gpu_objects.hpp"
#include "graphics/stk_tex_manager.hpp"
#include "guiengine/screen.hpp"
#include "guiengine/widgets/button_widget.hpp"
#include "guiengine/widgets/check_box_widget.hpp"
#include "guiengine/widgets/dynamic_ribbon_widget.hpp"
#include "guiengine/widgets/spinner_widget.hpp"
#include "guiengine/widget.hpp"
#include "io/file_manager.hpp"
#include "states_screens/dialogs/custom_video_settings.hpp"
#include "states_screens/options_screen_audio.hpp"
#include "states_screens/options_screen_input.hpp"
#include "states_screens/options_screen_ui.hpp"
#include "states_screens/state_manager.hpp"
#include "states_screens/user_screen.hpp"
#include "utils/string_utils.hpp"
#include "utils/translation.hpp"

#include <iostream>
#include <sstream>

using namespace GUIEngine;

DEFINE_SCREEN_SINGLETON( OptionsScreenVideo );

// ----------------------------------------------------------------------------
void OptionsScreenVideo::initPresets()
{
    m_presets.push_back
    ({
        false /* light */, 0 /* shadow */, false /* bloom */, false /* motionblur */,
        false /* lightshaft */, false /* glow */, false /* mlaa */, false /* ssao */, false /* weather */,
        false /* animatedScenery */, 0 /* animatedCharacters */, 0 /* image_quality */,
        false /* depth of field */, false /* global illumination */, true /* degraded IBL */
    });

    m_presets.push_back
    ({
        false /* light */, 0 /* shadow */, false /* bloom */, false /* motionblur */,
        false /* lightshaft */, false /* glow */, false /* mlaa */, false /* ssao */, false /* weather */,
        true /* animatedScenery */, 1 /* animatedCharacters */, 1 /* image_quality */,
        false /* depth of field */, false /* global illumination */, true /* degraded IBL */
    });

    m_presets.push_back
    ({
        true /* light */, 0 /* shadow */, false /* bloom */, false /* motionblur */,
        false /* lightshaft */, false /* glow */, false /* mlaa */, false /* ssao */, true /* weather */,
        true /* animatedScenery */, 1 /* animatedCharacters */, 2 /* image_quality */,
        false /* depth of field */, false /* global illumination */, true /* degraded IBL */
    });

    m_presets.push_back
    ({
        true /* light */, 0 /* shadow */, false /* bloom */, true /* motionblur */,
        true /* lightshaft */, true /* glow */, true /* mlaa */, false /* ssao */, true /* weather */,
        true /* animatedScenery */, 1 /* animatedCharacters */, 2 /* image_quality */,
        false /* depth of field */, false /* global illumination */, false /* degraded IBL */
    });

    m_presets.push_back
    ({
        true /* light */, 512 /* shadow */, true /* bloom */, true /* motionblur */,
        true /* lightshaft */, true /* glow */, true /* mlaa */, true /* ssao */, true /* weather */,
        true /* animatedScenery */,
#ifndef SERVER_ONLY
        (SharedGPUObjects::getMaxMat4Size() > 512 || !CVS->supportsHardwareSkinning() ? 2 : 1),
#else
        2 /* animatedCharacters */,
#endif
        3 /* image_quality */,
        true /* depth of field */, false /* global illumination */, false /* degraded IBL */
    });

    m_presets.push_back
    ({
        true /* light */, 1024 /* shadow */, true /* bloom */, true /* motionblur */,
        true /* lightshaft */, true /* glow */, true /* mlaa */, true /* ssao */, true /* weather */,
        true /* animatedScenery */,
#ifndef SERVER_ONLY
        (SharedGPUObjects::getMaxMat4Size() > 512 || !CVS->supportsHardwareSkinning() ? 2 : 1),
#else
        2 /* animatedCharacters */,
#endif
        3 /* image_quality */,
        true /* depth of field */, true /* global illumination */, false /* degraded IBL */
    });

}   // initPresets
// ----------------------------------------------------------------------------

struct Resolution
{
    int width, height;

    Resolution()
    {
        width = 0;
        height = 0;
    }

    Resolution(int w, int h)
    {
        width = w;
        height = h;
    }

    bool operator< (Resolution r) const
    {
        return width < r.width || (width == r.width && height < r.height);
    }

    float getRatio() const
    {
        return (float) width / height;
    }
};

// ----------------------------------------------------------------------------
int OptionsScreenVideo::getImageQuality()
{
    if (UserConfigParams::m_trilinear == false &&
        UserConfigParams::m_anisotropic == 0 &&
        (UserConfigParams::m_high_definition_textures & 0x01) == 0x00 &&
        UserConfigParams::m_hq_mipmap == false)
        return 0;
    if (UserConfigParams::m_trilinear == true &&
        UserConfigParams::m_anisotropic == 2 &&
        (UserConfigParams::m_high_definition_textures & 0x01) == 0x00 &&
        UserConfigParams::m_hq_mipmap == false)
        return 1;
    if (UserConfigParams::m_trilinear == true &&
        UserConfigParams::m_anisotropic == 4 &&
        (UserConfigParams::m_high_definition_textures & 0x01) == 0x01 &&
        UserConfigParams::m_hq_mipmap == false)
        return 2;
    if (UserConfigParams::m_trilinear == true &&
        UserConfigParams::m_anisotropic == 16 &&
        (UserConfigParams::m_high_definition_textures & 0x01) == 0x01 &&
        UserConfigParams::m_hq_mipmap == true)
        return 3;
    return 2;
}   // getImageQuality

// ----------------------------------------------------------------------------
void OptionsScreenVideo::setImageQuality(int quality)
{
    switch (quality)
    {
        case 0:
            UserConfigParams::m_trilinear = false;
            UserConfigParams::m_anisotropic = 0;
            UserConfigParams::m_high_definition_textures = 0x02;
            UserConfigParams::m_hq_mipmap = false;
            break;
        case 1:
            UserConfigParams::m_trilinear = true;
            UserConfigParams::m_anisotropic = 2;
            UserConfigParams::m_high_definition_textures = 0x02;
            UserConfigParams::m_hq_mipmap = false;
            break;
        case 2:
            UserConfigParams::m_trilinear = true;
            UserConfigParams::m_anisotropic = 4;
            UserConfigParams::m_high_definition_textures = 0x03;
            UserConfigParams::m_hq_mipmap = false;
            break;
        case 3:
            UserConfigParams::m_trilinear = true;
            UserConfigParams::m_anisotropic = 16;
            UserConfigParams::m_high_definition_textures = 0x03;
            UserConfigParams::m_hq_mipmap = true;
            break;
        default:
            assert(false);
    }
}   // setImageQuality

// ----------------------------------------------------------------------------

OptionsScreenVideo::OptionsScreenVideo() : Screen("options_video.stkgui"),
                                           m_prev_adv_pipline(false),
                                           m_prev_img_quality(-1)
{
    m_inited = false;
    initPresets();
}   // OptionsScreenVideo

// ----------------------------------------------------------------------------

void OptionsScreenVideo::loadedFromFile()
{
    m_inited = false;
    assert(m_presets.size() == 6);

    GUIEngine::SpinnerWidget* gfx =
        getWidget<GUIEngine::SpinnerWidget>("gfx_level");
    gfx->m_properties[GUIEngine::PROP_MAX_VALUE] =
        StringUtils::toString(m_presets.size());

}   // loadedFromFile

// ----------------------------------------------------------------------------

void OptionsScreenVideo::init()
{
    Screen::init();
    m_prev_adv_pipline = UserConfigParams::m_dynamic_lights;
    m_prev_img_quality = getImageQuality();
    RibbonWidget* ribbon = getWidget<RibbonWidget>("options_choice");
    assert(ribbon != NULL);
    ribbon->select( "tab_video", PLAYER_ID_GAME_MASTER );

    ribbon->getRibbonChildren()[1].setTooltip( _("Audio") );
    ribbon->getRibbonChildren()[2].setTooltip( _("User Interface") );
    ribbon->getRibbonChildren()[3].setTooltip( _("Players") );
    ribbon->getRibbonChildren()[4].setTooltip( _("Controls") );

    GUIEngine::ButtonWidget* applyBtn =
        getWidget<GUIEngine::ButtonWidget>("apply_resolution");
    assert( applyBtn != NULL );

    GUIEngine::SpinnerWidget* gfx =
        getWidget<GUIEngine::SpinnerWidget>("gfx_level");
    assert( gfx != NULL );

    GUIEngine::CheckBoxWidget* vsync =
        getWidget<GUIEngine::CheckBoxWidget>("vsync");
    assert( vsync != NULL );
    vsync->setState( UserConfigParams::m_vsync );


    // ---- video modes
    DynamicRibbonWidget* res = getWidget<DynamicRibbonWidget>("resolutions");
    assert( res != NULL );


    CheckBoxWidget* full = getWidget<CheckBoxWidget>("fullscreen");
    assert( full != NULL );
    full->setState( UserConfigParams::m_fullscreen );

    CheckBoxWidget* rememberWinpos = getWidget<CheckBoxWidget>("rememberWinpos");
    rememberWinpos->setState(UserConfigParams::m_remember_window_location);

    rememberWinpos->setActive(!UserConfigParams::m_fullscreen);

    // --- get resolution list from irrlicht the first time
    if (!m_inited)
    {
        res->clearItems();

        const std::vector<IrrDriver::VideoMode>& modes =
                                                irr_driver->getVideoModes();
        const int amount = (int)modes.size();

        std::vector<Resolution> resolutions;
        Resolution r;

        bool found_config_res = false;

        // for some odd reason, irrlicht sometimes fails to report the good
        // old standard resolutions
        // those are always useful for windowed mode
        bool found_1024_768 = false;

        for (int n=0; n<amount; n++)
        {
            r.width  = modes[n].getWidth();
            r.height = modes[n].getHeight();
            resolutions.push_back(r);

            if (r.width  == UserConfigParams::m_width &&
                r.height == UserConfigParams::m_height)
            {
                found_config_res = true;
            }

            if (r.width == 1024 && r.height == 768)
            {
                found_1024_768 = true;
            }
        }

#ifndef ANDROID
        if (!found_config_res)
        {
            r.width  = UserConfigParams::m_width;
            r.height = UserConfigParams::m_height;
            resolutions.push_back(r);

            if (r.width == 1024 && r.height == 768)
            {
                found_1024_768 = true;
            }
        } // next found resolution

        // Add default resolutions that were not found by irrlicht
        if (!found_1024_768)
        {
            r.width  = 1024;
            r.height = 768;
            resolutions.push_back(r);
        }
#endif

        // Sort resolutions by size
        std::sort(resolutions.begin(), resolutions.end());

        // Add resolutions list
        for(std::vector<Resolution>::iterator it = resolutions.begin();
            it != resolutions.end(); it++)
        {
            const float ratio = it->getRatio();
            char name[32];
            sprintf(name, "%ix%i", it->width, it->height);

            core::stringw label;
            label += it->width;
            label += L"\u00D7";
            label += it->height;

#define ABOUT_EQUAL(a , b) (fabsf( a - b ) < 0.01)

            if      (ABOUT_EQUAL( ratio, (5.0f/4.0f) ))
                res->addItem(label, name, "/gui/screen54.png");
            else if (ABOUT_EQUAL( ratio, (4.0f/3.0f) ))
                res->addItem(label, name, "/gui/screen43.png");
            else if (ABOUT_EQUAL( ratio, (16.0f/10.0f)))
                res->addItem(label, name, "/gui/screen1610.png");
            else if (ABOUT_EQUAL( ratio, (5.0f/3.0f) ))
                res->addItem(label, name, "/gui/screen53.png");
            else if (ABOUT_EQUAL( ratio, (3.0f/2.0f) ))
                res->addItem(label, name, "/gui/screen32.png");
            else if (ABOUT_EQUAL( ratio, (16.0f/9.0f) ))
                res->addItem(label, name, "/gui/screen169.png");
            else
                res->addItem(label, name, "/gui/screen_other.png");
#undef ABOUT_EQUAL
        } // add next resolution
    } // end if not inited

    res->updateItemDisplay();

    // ---- select current resolution every time
    char searching_for[32];
    snprintf(searching_for, 32, "%ix%i", (int)UserConfigParams::m_width,
                                         (int)UserConfigParams::m_height);


    if (!res->setSelection(searching_for, PLAYER_ID_GAME_MASTER,
                          false /* focus it */, true /* even if deactivated*/))
    {
        Log::error("OptionsScreenVideo", "Cannot find resolution %s", searching_for);
    }


    // --- set gfx settings values
    updateGfxSlider();

    // ---- forbid changing resolution or animation settings from in-game
    // (we need to disable them last because some items can't be edited when
    // disabled)
    bool in_game = StateManager::get()->getGameState() == GUIEngine::INGAME_MENU;

    res->setActive(!in_game);
    full->setActive(!in_game);
    applyBtn->setActive(!in_game);
    gfx->setActive(!in_game);
    getWidget<ButtonWidget>("custom")->setActive(!in_game);
}   // init

// ----------------------------------------------------------------------------

void OptionsScreenVideo::updateGfxSlider()
{
    GUIEngine::SpinnerWidget* gfx =
    getWidget<GUIEngine::SpinnerWidget>("gfx_level");
    assert( gfx != NULL );

    bool found = false;
    for (unsigned int l = 0; l < m_presets.size(); l++)
    {
        if (m_presets[l].animatedCharacters == UserConfigParams::m_show_steering_animations &&
            m_presets[l].animatedScenery == UserConfigParams::m_graphical_effects &&
            m_presets[l].image_quality == getImageQuality() &&
            m_presets[l].bloom == UserConfigParams::m_bloom &&
            m_presets[l].glow == UserConfigParams::m_glow &&
            m_presets[l].lights == UserConfigParams::m_dynamic_lights &&
            m_presets[l].lightshaft == UserConfigParams::m_light_shaft &&
            m_presets[l].mlaa == UserConfigParams::m_mlaa &&
            m_presets[l].motionblur == UserConfigParams::m_motionblur &&
            //m_presets[l].shaders == UserConfigParams::m_pixel_shaders
            m_presets[l].shadows == UserConfigParams::m_shadows_resolution &&
            m_presets[l].ssao == UserConfigParams::m_ssao &&
            m_presets[l].weather == UserConfigParams::m_weather_effects &&
            m_presets[l].dof == UserConfigParams::m_dof &&
            m_presets[l].global_illumination == UserConfigParams::m_gi &&
            m_presets[l].degraded_ibl == UserConfigParams::m_degraded_IBL)
        {
            gfx->setValue(l + 1);
            found = true;
            break;
        }
    }

    if (!found)
    {
        //I18N: custom video settings
        gfx->setCustomText( _("Custom") );
    }

    updateTooltip();
}

// ----------------------------------------------------------------------------

void OptionsScreenVideo::updateTooltip()
{
    GUIEngine::SpinnerWidget* gfx =
        getWidget<GUIEngine::SpinnerWidget>("gfx_level");
    assert( gfx != NULL );

    core::stringw tooltip;

    //I18N: in the graphical options tooltip;
    // indicates a graphical feature is enabled
    const core::stringw enabled = _LTR("Enabled");
    //I18N: in the graphical options tooltip;
    // indicates a graphical feature is disabled
    const core::stringw disabled = _LTR("Disabled");
    //I18N: if all kart animations are enabled
    const core::stringw all = _LTR("All");
    //I18N: if some kart animations are enabled
    const core::stringw me = _LTR("Me Only");
    //I18N: if no kart animations are enabled
    const core::stringw none = _LTR("None");

    //I18N: in the graphical options tooltip;
    // indicates the rendered image quality is very low
    const core::stringw very_low = _LTR("Very Low");
    //I18N: in the graphical options tooltip;
    // indicates the rendered image quality is low
    const core::stringw low = _LTR("Low");
    //I18N: in the graphical options tooltip;
    // indicates the rendered image quality is high
    const core::stringw high = _LTR("High");
    //I18N: in the graphical options tooltip;
    // indicates the rendered image quality is very high
    const core::stringw very_high = _LTR("Very High");

    //I18N: in graphical options
//    tooltip = tooltip + L"\n" + _("Pixel shaders: %s",
//                                  UserConfigParams::m_pixel_shaders ? enabled : disabled);
    //I18N: in graphical options
    tooltip = _("Animated Scenery: %s",
        UserConfigParams::m_graphical_effects ? enabled : disabled);
    //I18N: in graphical options
    tooltip = tooltip + L"\n" + _("Weather Effects: %s",
        UserConfigParams::m_weather_effects ? enabled : disabled);
    //I18N: in graphical options
    tooltip = tooltip + L"\n" + _("Animated Characters: %s",
        UserConfigParams::m_show_steering_animations == 2
        ? all
        : (UserConfigParams::m_show_steering_animations == 1 ? me : none));
    //I18N: in graphical options
    tooltip = tooltip + L"\n" + _("Dynamic lights: %s",
        UserConfigParams::m_dynamic_lights ? enabled : disabled);
    //I18N: in graphical options
    tooltip = tooltip + L"\n" + _("Motion blur: %s",
        UserConfigParams::m_motionblur ? enabled : disabled);
    //I18N: in graphical options
    tooltip = tooltip + L"\n" + _("Anti-aliasing: %s",
        UserConfigParams::m_mlaa ? enabled : disabled);
    //I18N: in graphical options
    tooltip = tooltip + L"\n" + _("Ambient occlusion: %s",
        UserConfigParams::m_ssao ? enabled : disabled);
    //I18N: in graphical options
    if (UserConfigParams::m_shadows_resolution == 0)
        tooltip = tooltip + L"\n" + _("Shadows: %s", disabled);
    else
        tooltip = tooltip + L"\n" + _("Shadows: %i", UserConfigParams::m_shadows_resolution);

    //I18N: in graphical options
    tooltip = tooltip + L"\n" + _("Bloom: %s",
        UserConfigParams::m_bloom ? enabled : disabled);

    //I18N: in graphical options
    tooltip = tooltip + L"\n" + _("Glow (outlines): %s",
        UserConfigParams::m_glow ? enabled : disabled);

    //I18N: in graphical options
    tooltip = tooltip + L"\n" + _("Light shaft (God rays): %s",
        UserConfigParams::m_light_shaft ? enabled : disabled);

    //I18N: in graphical options
    tooltip = tooltip + L"\n" + _("Depth of field: %s",
        UserConfigParams::m_dof ? enabled : disabled);

    //I18N: in graphical options
    tooltip = tooltip + L"\n" + _("Global illumination: %s",
        UserConfigParams::m_gi ? enabled : disabled);
    
    //I18N: in graphical options
    int quality = getImageQuality();
    tooltip = tooltip + L"\n" + _("Rendered image quality: %s",
        quality == 0 ? very_low : quality == 1 ? low : quality == 2 ?
        high : very_high);

    gfx->setTooltip(tooltip);
}   // updateTooltip

// ----------------------------------------------------------------------------

void OptionsScreenVideo::eventCallback(Widget* widget, const std::string& name,
                                       const int playerID)
{
    if (name == "options_choice")
    {
        std::string selection = ((RibbonWidget*)widget)->getSelectionIDString(PLAYER_ID_GAME_MASTER);

        Screen *screen = NULL;
        if (selection == "tab_audio")
            screen = OptionsScreenAudio::getInstance();
        //else if (selection == "tab_video")
        //    screen = OptionsScreenVideo::getInstance();
        else if (selection == "tab_players")
            screen = TabbedUserScreen::getInstance();
        else if (selection == "tab_controls")
            screen = OptionsScreenInput::getInstance();
        else if (selection == "tab_ui")
            screen = OptionsScreenUI::getInstance();
        if(screen)
            StateManager::get()->replaceTopMostScreen(screen);
    }
    else if(name == "back")
    {
        StateManager::get()->escapePressed();
    }
    else if(name == "custom")
    {
        new CustomVideoSettingsDialog(0.8f, 0.9f);
    }
    else if(name == "apply_resolution")
    {
        using namespace GUIEngine;

        DynamicRibbonWidget* w1=getWidget<DynamicRibbonWidget>("resolutions");
        assert(w1 != NULL);

        const std::string& res =
            w1->getSelectionIDString(PLAYER_ID_GAME_MASTER);

        int w = -1, h = -1;
        if (sscanf(res.c_str(), "%ix%i", &w, &h) != 2 || w == -1 || h == -1)
        {
            Log::error("OptionsScreenVideo", "Failed to decode resolution %s", res.c_str());
            return;
        }

        CheckBoxWidget* w2 = getWidget<CheckBoxWidget>("fullscreen");
        assert(w2 != NULL);


        irr_driver->changeResolution(w, h, w2->getState());
    }
    else if (name == "gfx_level")
    {
        GUIEngine::SpinnerWidget* gfx_level =
            getWidget<GUIEngine::SpinnerWidget>("gfx_level");
        assert( gfx_level != NULL );

        const int level = gfx_level->getValue() - 1;

        UserConfigParams::m_show_steering_animations = m_presets[level].animatedCharacters;
        UserConfigParams::m_graphical_effects = m_presets[level].animatedScenery;
        setImageQuality(m_presets[level].image_quality);
        UserConfigParams::m_bloom = m_presets[level].bloom;
        UserConfigParams::m_glow = m_presets[level].glow;
        UserConfigParams::m_dynamic_lights = m_presets[level].lights;
        UserConfigParams::m_light_shaft = m_presets[level].lightshaft;
        UserConfigParams::m_mlaa = m_presets[level].mlaa;
        UserConfigParams::m_motionblur = m_presets[level].motionblur;
        //UserConfigParams::m_pixel_shaders = m_presets[level].shaders;
        UserConfigParams::m_shadows_resolution = m_presets[level].shadows;
        UserConfigParams::m_ssao = m_presets[level].ssao;
        UserConfigParams::m_weather_effects = m_presets[level].weather;
        UserConfigParams::m_dof = m_presets[level].dof;
        UserConfigParams::m_gi = m_presets[level].global_illumination;
        UserConfigParams::m_degraded_IBL = m_presets[level].degraded_ibl;

        updateGfxSlider();
    }
    else if (name == "vsync")
    {
        GUIEngine::CheckBoxWidget* vsync =
            getWidget<GUIEngine::CheckBoxWidget>("vsync");
        assert( vsync != NULL );
        UserConfigParams::m_vsync = vsync->getState();
    }
    else if (name == "rememberWinpos")
    {
        CheckBoxWidget* rememberWinpos = getWidget<CheckBoxWidget>("rememberWinpos");
        UserConfigParams::m_remember_window_location = rememberWinpos->getState();
    }
    else if (name == "fullscreen")
    {
        CheckBoxWidget* fullscreen = getWidget<CheckBoxWidget>("fullscreen");
        CheckBoxWidget* rememberWinpos = getWidget<CheckBoxWidget>("rememberWinpos");

        rememberWinpos->setActive(!fullscreen->getState());
    }
}   // eventCallback

// ----------------------------------------------------------------------------

void OptionsScreenVideo::tearDown()
{
    if (m_prev_adv_pipline != UserConfigParams::m_dynamic_lights)
        irr_driver->sameRestart();
    else if (m_prev_img_quality != getImageQuality())
    {
        irr_driver->setMaxTextureSize();
        STKTexManager::getInstance()->destroyThreadedTexLoaders();
        STKTexManager::getInstance()->createThreadedTexLoaders();
    }
    Screen::tearDown();
    // save changes when leaving screen
    user_config->saveConfig();
}   // tearDown

// ----------------------------------------------------------------------------

void OptionsScreenVideo::unloaded()
{
    m_inited = false;
}   // unloaded

// ----------------------------------------------------------------------------

