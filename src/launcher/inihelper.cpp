#include "inihelper.h"

void writeDefaultDdrawIni(mINI::INIFile& file, mINI::INIStructure& ini) {
    // Check if ini file is empty
    if (ini.size() == 0) {
        // Write hardcoded keys and values to ini file
        ini["ddraw"]["width"] = "0";
        ini["ddraw"]["height"] = "0";
        ini["ddraw"]["fullscreen"] = "true";
        ini["ddraw"]["windowed"] = "false";
        ini["ddraw"]["maintas"] = "false";
        ini["ddraw"]["boxing"] = "false";
        ini["ddraw"]["maxfps"] = "60";
        ini["ddraw"]["vsync"] = "false";
        ini["ddraw"]["adjmouse"] = "false";
        ini["ddraw"]["shader"] = "Shaders\\xbr\\xbr-lv2-noblend.glsl";
        ini["ddraw"]["posX"] = "-32000";
        ini["ddraw"]["posY"] = "-32000";
        ini["ddraw"]["renderer"] = "opengl";
        ini["ddraw"]["devmode"] = "false";
        ini["ddraw"]["border"] = "false";
        ini["ddraw"]["savesettings"] = "1";
        ini["ddraw"]["resizeable"] = "true";
        ini["ddraw"]["noactivateapp"] = "false";
        ini["ddraw"]["maxgameticks"] = "-2";
        ini["ddraw"]["handlemouse"] = "true";
        ini["ddraw"]["hook"] = "4";
        ini["ddraw"]["minfps"] = "0";
        ini["ddraw"]["nonexclusive"] = "false";
        ini["ddraw"]["singlecpu"] = "true";
        ini["ddraw"]["vhack"] = "";
        ini["ddraw"]["d3d9linear"] = "";

        file.write(ini);
        return;
    }
}