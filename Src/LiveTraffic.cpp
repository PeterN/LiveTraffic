/*
 * Copyright (c) 2018, Birger Hoppe
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Uses
 * - XPlaneMP, see https://github.com/kuroneko/libxplanemp and http://xsb.xsquawkbox.net/developers/
 * - libpng 1.2.59, see http://www.libpng.org/pub/png/libpng.html
 */

// All includes are collected in one header
#include "LiveTraffic.h"

// in LTVersion.cpp:
extern bool InitFullVersion ();

//MARK: Globals

// access to data refs
DataRefs dataRefs(logWARN);

// Settings Dialog
LTSettingsUI settingsUI;

//MARK: Reload Plugins menu item
enum menuItems {
    MENU_ID_AC_INFO_WND = 0,
    MENU_ID_AC_INFO_WND_POPOUT,
    MENU_ID_TOGGLE_AIRCRAFTS,
    MENU_ID_HAVE_TCAS,
    MENU_ID_SETTINGS_UI,
#ifdef DEBUG
    MENU_ID_RELOAD_PLUGINS,
#endif
    CNT_MENU_ID                     // always last, number of elements
};

// ID of the "LiveTraffic" menu within the plugin menu; items numbers of its menu items
XPLMMenuID menuID = 0;
int aMenuItems[CNT_MENU_ID];

// Callback called by XP, so this is an entry point into the plugin
void MenuHandler(void * /*mRef*/, void * iRef)
{
    // LiveTraffic top level exception handling
    try {
        // act based on menu id
        switch (reinterpret_cast<unsigned long long>(iRef)) {
            case MENU_ID_AC_INFO_WND:
                ACIWnd::OpenNewWnd(!XPLMHasFeature("XPLM_USE_NATIVE_WIDGET_WINDOWS") ?
                                   TF_MODE_CLASSIC :
                                   dataRefs.IsVREnabled() ?
                                   TF_MODE_VR : TF_MODE_FLOAT);
                break;
            case MENU_ID_AC_INFO_WND_POPOUT:
                ACIWnd::OpenNewWnd(TF_MODE_POPOUT);
                break;
            case MENU_ID_TOGGLE_AIRCRAFTS:
                dataRefs.ToggleAircraftsDisplayed();
                break;
            case MENU_ID_HAVE_TCAS:
                LTMainTryGetAIAircraft();
                break;
            case MENU_ID_SETTINGS_UI:
                settingsUI.Show();
                settingsUI.Center();
                break;
#ifdef DEBUG
            case MENU_ID_RELOAD_PLUGINS:
                XPLMReloadPlugins();
                break;
#endif
        }
    } catch (const std::exception& e) {
        LOG_MSG(logERR, ERR_TOP_LEVEL_EXCEPTION, e.what());
        // otherwise ignore
    } catch (...) {
        // ignore
    }
}

// the "Aircrafts displayed" menu item includes the number of displayed a/c
// (if the item is checked, i.e. active)
void MenuCheckAircraftsDisplayed ( bool bChecked, int numAc )
{
    XPLMCheckMenuItem(// checkmark the menu item if aircrafts shown
                      menuID,aMenuItems[MENU_ID_TOGGLE_AIRCRAFTS],
                      bChecked ? xplm_Menu_Checked : xplm_Menu_Unchecked);
    
    // update menu item's name with number of a/c
    // (if there are a/c, or is item enabled)
    if (bChecked || numAc > 0)
    {
        char szItemName[50];
        snprintf(szItemName, sizeof(szItemName), MENU_TOGGLE_AC_NUM, numAc);
        XPLMSetMenuItemName(menuID, aMenuItems[MENU_ID_TOGGLE_AIRCRAFTS],
                            szItemName, 0);
    }
        else
    {
        XPLMSetMenuItemName(menuID, aMenuItems[MENU_ID_TOGGLE_AIRCRAFTS],
                            MENU_TOGGLE_AIRCRAFTS, 0);
    }
}

void MenuCheckTCASControl ( bool bChecked )
{
    XPLMCheckMenuItem(// checkmark the menu item if TCAS under control
                      menuID,aMenuItems[MENU_ID_HAVE_TCAS],
                      bChecked ? xplm_Menu_Checked : xplm_Menu_Unchecked);
}

int RegisterMenuItem ()
{
    // Create menu and menu item
    int item = XPLMAppendMenuItem(XPLMFindPluginsMenu(), LIVE_TRAFFIC, NULL, 1);
    if ( item<0 ) { LOG_MSG(logERR,ERR_APPEND_MENU_ITEM); return 0; }
    
    menuID = XPLMCreateMenu(LIVE_TRAFFIC, XPLMFindPluginsMenu(), item, MenuHandler, NULL);
    if ( !menuID ) { LOG_MSG(logERR,ERR_CREATE_MENU); return 0; }
    
    // clear menu array
    memset(aMenuItems, 0, sizeof(aMenuItems));
    
    // Open an aircraft info window
    aMenuItems[MENU_ID_AC_INFO_WND] =
    XPLMAppendMenuItem(menuID, MENU_AC_INFO_WND, (void *)MENU_ID_AC_INFO_WND,1);
    if ( aMenuItems[MENU_ID_AC_INFO_WND]<0 ) { LOG_MSG(logERR,ERR_APPEND_MENU_ITEM); return 0; }
    
    // modern windows only if available
    if (XPLMHasFeature("XPLM_USE_NATIVE_WIDGET_WINDOWS")) {
        aMenuItems[MENU_ID_AC_INFO_WND_POPOUT] =
        XPLMAppendMenuItem(menuID, MENU_AC_INFO_WND_POPOUT, (void *)MENU_ID_AC_INFO_WND_POPOUT,1);
        if ( aMenuItems[MENU_ID_AC_INFO_WND_POPOUT]<0 ) { LOG_MSG(logERR,ERR_APPEND_MENU_ITEM); return 0; }
    }

    // Separator
    XPLMAppendMenuSeparator(menuID);
    
    // Show Aircrafts
    aMenuItems[MENU_ID_TOGGLE_AIRCRAFTS] =
    XPLMAppendMenuItem(menuID, MENU_TOGGLE_AIRCRAFTS, (void *)MENU_ID_TOGGLE_AIRCRAFTS,1);
    if ( aMenuItems[MENU_ID_TOGGLE_AIRCRAFTS]<0 ) { LOG_MSG(logERR,ERR_APPEND_MENU_ITEM); return 0; }
    // no checkmark symbol (but room for one later)
    XPLMCheckMenuItem(menuID,aMenuItems[MENU_ID_TOGGLE_AIRCRAFTS],xplm_Menu_Unchecked);
    
    // Have/Get TCAS
    aMenuItems[MENU_ID_HAVE_TCAS] =
    XPLMAppendMenuItem(menuID, MENU_HAVE_TCAS, (void *)MENU_ID_HAVE_TCAS,1);
    if ( aMenuItems[MENU_ID_HAVE_TCAS]<0 ) { LOG_MSG(logERR,ERR_APPEND_MENU_ITEM); return 0; }
    // no checkmark symbol (but room for one later)
    XPLMCheckMenuItem(menuID,aMenuItems[MENU_ID_HAVE_TCAS],xplm_Menu_Unchecked);
    
    // Separator
    XPLMAppendMenuSeparator(menuID);
    
    // Show Settings UI
    aMenuItems[MENU_ID_SETTINGS_UI] =
    XPLMAppendMenuItem(menuID, MENU_SETTINGS_UI, (void *)MENU_ID_SETTINGS_UI,1);
    if ( aMenuItems[MENU_ID_SETTINGS_UI]<0 ) { LOG_MSG(logERR,ERR_APPEND_MENU_ITEM); return 0; }
    
#ifdef DEBUG
    // Separator
    XPLMAppendMenuSeparator(menuID);
    
    // Reload Plugins
    aMenuItems[MENU_ID_RELOAD_PLUGINS] =
        XPLMAppendMenuItem(menuID, MENU_RELOAD_PLUGINS, (void *)MENU_ID_RELOAD_PLUGINS,1);
    if ( aMenuItems[MENU_ID_RELOAD_PLUGINS]<0 ) { LOG_MSG(logERR,ERR_APPEND_MENU_ITEM); return 0; }
#endif

    // Success
    LOG_MSG(logDEBUG,DBG_MENU_CREATED)
    return 1;
}

//MARK: XPlugin Callbacks
PLUGIN_API int XPluginStart(
							char *		outName,
							char *		outSig,
							char *		outDesc)
{
    try {
        // init our version number
        if (!InitFullVersion ()) return 0;

        // init random numbers
         srand((unsigned int)time(NULL));
        
        // tell X-Plane who we are
         strcpy_s(outName, 255, LIVE_TRAFFIC);
         strcpy_s(outSig,  255, PLUGIN_SIGNATURE);
         strcpy_s(outDesc, 255, PLUGIN_DESCRIPTION);
        
#ifdef DEBUG
        // install error handler
        XPLMSetErrorCallback(LTErrorCB);
#endif
        
        // tell the world we are trying to start up
        LOG_MSG(logMSG, MSG_STARTUP, LT_VERSION_FULL);
        
        // use native paths, i.e. Posix style (as opposed to HFS style)
        // https://developer.x-plane.com/2014/12/mac-plugin-developers-you-should-be-using-native-paths/
        XPLMEnableFeature("XPLM_USE_NATIVE_PATHS",1);

        // init DataRefs
        if (!dataRefs.Init()) { DestroyWindow(); return 0; }
        
        // read FlightModel.prf file (which we could live without)
        LTAircraft::FlightModel::ReadFlightModelFile();
        
        // init Aircraft handling (including XPMP)
        if (!LTMainInit()) { DestroyWindow(); return 0; }
        
        // create menu
        if (!RegisterMenuItem()) { DestroyWindow(); return 0; }
        
        // Success
        return 1;
        
    } catch (const std::exception& e) {
        LOG_MSG(logERR, ERR_TOP_LEVEL_EXCEPTION, e.what());
        return 0;
    } catch (...) {
        return 0;
    }
}

PLUGIN_API int  XPluginEnable(void)
{
    try {
        // Enable showing aircrafts
        if (!LTMainEnable()) return 0;

        // Create a message window and say hello
        SHOW_MSG(logMSG, MSG_WELCOME, LT_VERSION_FULL);
        if constexpr (VERSION_BETA)
            SHOW_MSG(logWARN, BETA_LIMITED_VERSION, LT_BETA_VER_LIMIT_TXT);
#ifdef DEBUG
        SHOW_MSG(logWARN, DBG_DEBUG_BUILD);
#endif
        
        // Success
        return 1;

    } catch (const std::exception& e) {
        LOG_MSG(logERR, ERR_TOP_LEVEL_EXCEPTION, e.what());
        return 0;
    } catch (...) {
        return 0;
    }
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void * /*inParam*/)
{
    // we only process msgs from X-Plane
    if (inFrom != XPLM_PLUGIN_XPLANE)
        return;
    
#ifdef DEBUG
    // for me not having a VR rig I do some basic testing with sending XPLM_MSG_AIRPLANE_COUNT_CHANGED
    static bool bDebugPretendVR = false;
    if (inMsg == XPLM_MSG_AIRPLANE_COUNT_CHANGED) {
        bDebugPretendVR = !bDebugPretendVR;
        inMsg = bDebugPretendVR ? XPLM_MSG_ENTERED_VR : XPLM_MSG_EXITING_VR;
    }
#endif
    
    switch (inMsg) {
        // *** entering VR mode ***
        case XPLM_MSG_ENTERED_VR:
            ACIWnd::MoveAllVR(true);
            break;
            
        // *** existing from VR mode ***
        case XPLM_MSG_EXITING_VR:
            ACIWnd::MoveAllVR(false);
            break;
    }
}

PLUGIN_API void XPluginDisable(void) {
    try {
        // if there still is a message window remove it
        DestroyWindow();
        
        // deregister Settings UI
        settingsUI.Disable();
        
        // stop showing aircrafts
        LTMainDisable ();

        // Meu item "Aircrafts displayed" no checkmark symbol (but room one later)
        XPLMCheckMenuItem(menuID,aMenuItems[MENU_ID_TOGGLE_AIRCRAFTS],xplm_Menu_Unchecked);

        LOG_MSG(logMSG, MSG_DISABLED);

    } catch (const std::exception& e) {
        LOG_MSG(logERR, ERR_TOP_LEVEL_EXCEPTION, e.what());
    } catch (...) {
    }
}

PLUGIN_API void    XPluginStop(void)
{
    try {
        // Cleanup aircraft handling (including XPMP library)
        LTMainStop();
        
        // Cleanup dataRef registration
        dataRefs.Stop();
        
        // last chance to remove the message area window
        DestroyWindow();

    } catch (const std::exception& e) {
        LOG_MSG(logERR, ERR_TOP_LEVEL_EXCEPTION, e.what());
    } catch (...) {
    }
}

#ifdef DEBUG
// Error callback, called by X-Plane if we you bogus data in our API calls
void LTErrorCB (const char* msg)
{
    char s[512];
    snprintf(s, sizeof(s), "%s FATAL ERROR CALLBACK: %s", LIVE_TRAFFIC, msg);
    XPLMDebugString(s);
    assert(msg!=NULL);          // we bail out and hope the debugger is attached, so we can debug where we come from
}
#endif
