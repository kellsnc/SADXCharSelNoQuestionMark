#include "pch.h"

enum CharSelType : uint8_t
{
    CharSelType_Adventure,
    CharSelType_Trial,
    CharSelType_Mission
};

DataPointer(CharSelType, charSelType, 0x3B2A2FA);
FunctionPointer(void, FreeCharacter, (int id), 0x44B010);

#pragma pack(push, 1)
struct CharSelDataA
{
    ObjectMaster* CharacterObjects[7];
    ObjectMaster* object;
    char gap_20[12];
    char field_2C;
    char CurrentCharAnim[7];
    char CharSelected;
    char gap_35[2];
    char field_37;
};
#pragma pack(pop)

#define BYTE1(x)   (*((uint8_t*)&(x)+1))

static Trampoline* ExtraCharacter_Load_t = nullptr;
static bool HudScale = false;

static NJS_ACTION ExtraCharacterAction{};

static void __cdecl ExtraCharacter_Display(ObjectMaster* obj)
{
    auto data = obj->Data1;

    if (!MissedFrames && BYTE1(CharacterSelection) == data->CharIndex)
    {
        auto data2 = reinterpret_cast<EntityData2*>(obj->Data2);
        auto co2 = data2->CharacterData;
        
        Direct3D_SetZFunc(1u);
        SetMaterialAndSpriteColor_Float(1.0f, 1.0f, 1.0f, 1.0f);

        njPushMatrixEx();
        
        if (charSelType == CharSelType_Adventure)
        {
            // SUPER SONIC

            int backup = SuperSonicFlag;
            njSetTexture(&SUPERSONIC_TEXLIST);
            Direct3D_PerformLighting(4);
            SuperSonicFlag = 1;
            njTranslate(nullptr, 0.0f, 3.0f, HudScale ? -20.0f : -20.0f * HorizontalStretch);
            njRotateY(nullptr, 0x4000);
            njAction(co2->AnimationThing.action, co2->AnimationThing.Frame);
            SuperSonicFlag = backup;
        }
        else
        {
            // METAL SONIC

            int backup = MetalSonicFlag;
            njSetTexture(&METALSONIC_TEXLIST);
            Direct3D_PerformLighting(2);
            MetalSonicFlag = 1;
            njTranslate(nullptr, 0.0f, data->Action == 2 ? 2.0f : 0.0f, HudScale ? -20.0f : -20.0f * HorizontalStretch);
            njRotateY(nullptr, 0x4000);
            njAction(co2->AnimationThing.action, co2->AnimationThing.Frame);
            MetalSonicFlag = backup;
        }

        njPopMatrixEx();
        Direct3D_PerformLighting(0);
        ClampGlobalColorThing_Thing();
        Direct3D_ResetZFunc();
    }
}

static void ExtraCharacterPlayAnim(EntityData1* data, CharObj2* co2, CharSelDataA* charseldata, int idleanim, int selectanim)
{
    if (charseldata->CharSelected == 0)
    {
        if (data->Action != 1)
        {
            ExtraCharacterAction.motion = SonicAnimData[idleanim].Animation->motion;
            co2->AnimationThing.Index = idleanim;
            co2->AnimationThing.Frame = 0.0f;
            data->Action = 1;
        }

        co2->AnimationThing.Frame = fmodf(co2->AnimationThing.Frame + 0.5f, co2->AnimationThing.action->motion->nbFrame - 1);
    }
    else
    {
        if (data->Action != 2)
        {
            ExtraCharacterAction.motion = SonicAnimData[selectanim].Animation->motion;
            co2->AnimationThing.Index = selectanim;
            co2->AnimationThing.Frame = 0.0f;
            data->Action = 2;
        }

        if (co2->AnimationThing.Frame < co2->AnimationThing.action->motion->nbFrame - 1)
        {
            co2->AnimationThing.Frame += 0.5f;
        }
        else
        {
            co2->AnimationThing.Frame = co2->AnimationThing.action->motion->nbFrame - 1;
        }
    }
}

static void __cdecl ExtraCharacter_Main(ObjectMaster* obj)
{
    auto data = obj->Data1;

    if (BYTE1(CharacterSelection) == data->CharIndex)
    {
        auto data2 = reinterpret_cast<EntityData2*>(obj->Data2);
        auto co2 = data2->CharacterData;

        auto charselobj = CharSel_ptr;
        auto charseldata = reinterpret_cast<CharSelDataA*>(charselobj->UnknownB_ptr);

        if (charSelType == CharSelType_Adventure)
        {
            // SUPER SONIC

            ExtraCharacterPlayAnim(data, co2, charseldata, 134, 143);
        }
        else
        {
            // METAL SONIC

            ExtraCharacterPlayAnim(data, co2, charseldata, 146, 75);
        }

        ProcessVertexWelds(data, data2, co2);
    }
    
    obj->DisplaySub(obj);
}

static void __cdecl ExtraCharacter_Delete(ObjectMaster* obj)
{
    auto data = obj->Data1;
    auto data2 = reinterpret_cast<EntityData2*>(obj->Data2);
    auto co2 = data2->CharacterData;

    co2->AnimationThing.action = nullptr;

    if (charSelType == CharSelType_Adventure)
    {
        njReleaseTexture(&SUPERSONIC_TEXLIST);
    }
    else
    {
        UnloadCharTextures(Characters_MetalSonic);
    }

    FreeCharacter(data->CharIndex);
}

static void __cdecl ExtraCharacter_Load(ObjectMaster* obj)
{
    if (ExtraCharacter_Load_t && charSelType == CharSelType_Adventure && !GetEventFlag(EventFlags_SuperSonicAdventureComplete))
    {
        obj->MainSub = reinterpret_cast<ObjectFuncPtr>(ExtraCharacter_Load_t->Target());
        return;
    }

    auto data = obj->Data1;
    auto data2 = reinterpret_cast<EntityData2*>(obj->Data2);

    // Load everything but CharBubble
    WriteData(reinterpret_cast<int*>(0x442961), 0);
    InitCharacterVars(data->CharIndex, obj);
    WriteData(reinterpret_cast<ObjectFuncPtr*>(0x442961), CharBubble_Init);

    auto co2 = data2->CharacterData;
    co2->AnimationThing.AnimData = SonicAnimData;

    if (charSelType == CharSelType_Adventure)
    {
        // SUPER SONIC

        WeldInfo* item = SonicWeldInfo;

        // Init disabled-by-default welds
        do
        {
            if (item->BaseModel)
            {
                if (item->BaseModel == SONIC_OBJECTS[22])
                {
                    item->WeldType = 2;
                }
            }

            ++item;
        }
        while (item->BaseModel);

        LoadPVM("SUPERSONIC", &SUPERSONIC_TEXLIST);
        co2->AnimationThing.WeldInfo = SonicWeldInfo;
        co2->Upgrades |= Upgrades_SuperSonic;

        ExtraCharacterAction.object = SONIC_OBJECTS[22];
    }
    else
    {
        // METAL SONIC

        LoadCharTextures(Characters_MetalSonic);
        co2->AnimationThing.WeldInfo = MetalSonicWeldInfo;

        ExtraCharacterAction.object = SONIC_OBJECTS[68];
    }

    ProcessVertexWelds(data, data2, co2);
    co2->AnimationThing.State = 2;
    co2->AnimationThing.action = &ExtraCharacterAction;

    obj->MainSub = ExtraCharacter_Main;
    obj->DisplaySub = ExtraCharacter_Display;
    obj->DeleteSub = ExtraCharacter_Delete;
}

extern "C"
{
	__declspec(dllexport) void __cdecl Init(const char* path, const HelperFunctions& helperFunctions)
	{
        const IniFile* config = new IniFile(std::string(path) + "\\config.ini");

        if (config->getBool("", "ShowIfCompleted", false))
        {
            ExtraCharacter_Load_t = new Trampoline(0x5125A0, 0x5125A6, ExtraCharacter_Load, false);
        }
        else
        {
            WriteJump(reinterpret_cast<void*>(0x5125A0), ExtraCharacter_Load);
        }

        delete config;

        const IniFile* loader_config = new IniFile("mods\\SADXModLoader.ini");
        HudScale = loader_config->getBool("", "ScaleHud", false);
        delete loader_config;
	}

	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer }; // This is needed for the Mod Loader to recognize the DLL.
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}