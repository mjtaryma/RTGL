// Copyright (c) 2022 Sultim Tsyrendashiev
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "VulkanDevice.h"

#include "Matrix.h"

#include "Generated/ShaderCommonC.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <ranges>

namespace
{

template< typename To, typename From >
To ClampPix( From v )
{
    return std::clamp( To( v ), To( 96 ), To( 3840 ) );
}

struct WholeWindow
{
    explicit WholeWindow( std::string_view name )
    {
#ifdef IMGUI_HAS_VIEWPORT
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos( viewport->WorkPos );
        ImGui::SetNextWindowSize( viewport->WorkSize );
        ImGui::SetNextWindowViewport( viewport->ID );
#else
        ImGui::SetNextWindowPos( ImVec2( 0.0f, 0.0f ) );
        ImGui::SetNextWindowSize( ImGui::GetIO().DisplaySize );
#endif
        ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );

        if( ImGui::Begin( name.data(),
                          nullptr,
                          ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                              ImGuiWindowFlags_NoBackground ) )
        {
            beginSuccess = ImGui::BeginTabBar( "##TabBar", ImGuiTabBarFlags_Reorderable );
        }
    }

    WholeWindow( const WholeWindow& other )                = delete;
    WholeWindow( WholeWindow&& other ) noexcept            = delete;
    WholeWindow& operator=( const WholeWindow& other )     = delete;
    WholeWindow& operator=( WholeWindow&& other ) noexcept = delete;

    explicit operator bool() const { return beginSuccess; }

    ~WholeWindow()
    {
        if( beginSuccess )
        {
            ImGui::EndTabBar();
        }
        ImGui::End();
        ImGui::PopStyleVar( 1 );
    }

private:
    bool beginSuccess{ false };
};

}

bool RTGL1::VulkanDevice::Dev_IsDevmodeInitialized() const
{
    return debugWindows && devmode;
}

void RTGL1::VulkanDevice::Dev_Draw() const
{
    if( !Dev_IsDevmodeInitialized() )
    {
        return;
    }

    if( debugWindows->IsMinimized() )
    {
        return;
    }

    auto w = WholeWindow( "Main window" );
    if( !w )
    {
        return;
    }

    if( ImGui::BeginTabItem( "General" ) )
    {
        ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.59f, 0.98f, 0.26f, 0.40f ) );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.59f, 0.98f, 0.26f, 1.00f ) );
        ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.53f, 0.98f, 0.06f, 1.00f ) );
        devmode->reloadShaders = ImGui::Button( "Reload shaders", { -1, 96 } );
        ImGui::PopStyleColor( 3 );

        auto& modifiers = devmode->drawInfoOvrd;

        ImGui::Dummy( ImVec2( 0, 4 ) );
        ImGui::Separator();
        ImGui::Dummy( ImVec2( 0, 4 ) );

        ImGui::Checkbox( "Override", &modifiers.enable );
        ImGui::BeginDisabled( !modifiers.enable );
        if( ImGui::TreeNode( "Present" ) )
        {
            ImGui::Checkbox( "Vsync", &modifiers.vsync );

            static_assert(
                std::is_same_v< int, std::underlying_type_t< RgRenderUpscaleTechnique > > );
            static_assert(
                std::is_same_v< int, std::underlying_type_t< RgRenderSharpenTechnique > > );
            static_assert(
                std::is_same_v< int, std::underlying_type_t< RgRenderResolutionMode > > );

            bool dlssOk = IsUpscaleTechniqueAvailable( RG_RENDER_UPSCALE_TECHNIQUE_NVIDIA_DLSS );
            {
                ImGui::RadioButton( "Linear##Upscale",
                                    reinterpret_cast< int* >( &modifiers.upscaleTechnique ),
                                    RG_RENDER_UPSCALE_TECHNIQUE_LINEAR );
                ImGui::SameLine();
                ImGui::RadioButton( "Nearest##Upscale",
                                    reinterpret_cast< int* >( &modifiers.upscaleTechnique ),
                                    RG_RENDER_UPSCALE_TECHNIQUE_NEAREST );
                ImGui::SameLine();
                ImGui::RadioButton( "FSR 2.1##Upscale",
                                    reinterpret_cast< int* >( &modifiers.upscaleTechnique ),
                                    RG_RENDER_UPSCALE_TECHNIQUE_AMD_FSR2 );
                ImGui::SameLine();
                ImGui::BeginDisabled( !dlssOk );
                ImGui::RadioButton( "DLSS 2##Upscale",
                                    reinterpret_cast< int* >( &modifiers.upscaleTechnique ),
                                    RG_RENDER_UPSCALE_TECHNIQUE_NVIDIA_DLSS );
                ImGui::EndDisabled();
            }
            {
                ImGui::RadioButton( "None##Sharp",
                                    reinterpret_cast< int* >( &modifiers.sharpenTechnique ),
                                    RG_RENDER_SHARPEN_TECHNIQUE_NONE );
                ImGui::SameLine();
                ImGui::RadioButton( "Naive sharpening##Sharp",
                                    reinterpret_cast< int* >( &modifiers.sharpenTechnique ),
                                    RG_RENDER_SHARPEN_TECHNIQUE_NAIVE );
                ImGui::SameLine();
                ImGui::RadioButton( "AMD CAS sharpening##Sharp",
                                    reinterpret_cast< int* >( &modifiers.sharpenTechnique ),
                                    RG_RENDER_SHARPEN_TECHNIQUE_AMD_CAS );
            }

            bool forceCustom =
                modifiers.upscaleTechnique != RG_RENDER_UPSCALE_TECHNIQUE_AMD_FSR2 &&
                modifiers.upscaleTechnique != RG_RENDER_UPSCALE_TECHNIQUE_NVIDIA_DLSS;
            if( forceCustom )
            {
                modifiers.resolutionMode = RG_RENDER_RESOLUTION_MODE_CUSTOM;
            }

            {
                ImGui::RadioButton( "Custom##Resolution",
                                    reinterpret_cast< int* >( &modifiers.resolutionMode ),
                                    RG_RENDER_RESOLUTION_MODE_CUSTOM );
                ImGui::SameLine();
                ImGui::BeginDisabled( forceCustom );
                ImGui::RadioButton( "Ultra Performance##Resolution",
                                    reinterpret_cast< int* >( &modifiers.resolutionMode ),
                                    RG_RENDER_RESOLUTION_MODE_ULTRA_PERFORMANCE );
                ImGui::SameLine();
                ImGui::RadioButton( "Performance##Resolution",
                                    reinterpret_cast< int* >( &modifiers.resolutionMode ),
                                    RG_RENDER_RESOLUTION_MODE_PERFORMANCE );
                ImGui::SameLine();
                ImGui::RadioButton( "Balanced##Resolution",
                                    reinterpret_cast< int* >( &modifiers.resolutionMode ),
                                    RG_RENDER_RESOLUTION_MODE_BALANCED );
                ImGui::SameLine();
                ImGui::RadioButton( "Quality##Resolution",
                                    reinterpret_cast< int* >( &modifiers.resolutionMode ),
                                    RG_RENDER_RESOLUTION_MODE_QUALITY );
                if( modifiers.upscaleTechnique == RG_RENDER_UPSCALE_TECHNIQUE_NVIDIA_DLSS )
                {
                    ImGui::SameLine();
                    ImGui::RadioButton( "DLAA##Resolution",
                                        reinterpret_cast< int* >( &modifiers.resolutionMode ),
                                        RG_RENDER_RESOLUTION_MODE_DLAA );
                }
                ImGui::EndDisabled();
            }
            {
                ImGui::BeginDisabled(
                    !( modifiers.resolutionMode == RG_RENDER_RESOLUTION_MODE_CUSTOM ) );

                ImGui::SliderFloat(
                    "Custom render size", &modifiers.customRenderSizeScale, 0.1f, 1.5f );

                ImGui::EndDisabled();
            }
            {
                ImGui::Checkbox( "Downscale to pixelized", &modifiers.pixelizedEnable );
                if( modifiers.pixelizedEnable )
                {
                    ImGui::SliderInt( "Pixelization size", &modifiers.pixelizedHeight, 100, 600 );
                }
            }

            ImGui::TreePop();
        }
        if( ImGui::TreeNode( "Tonemapping" ) )
        {
            ImGui::Checkbox( "Disable eye adaptation", &modifiers.disableEyeAdaptation );
            ImGui::SliderFloat( "EV100 min", &modifiers.ev100Min, -3, 16, "%.1f" );
            ImGui::SliderFloat( "EV100 max", &modifiers.ev100Max, -3, 16, "%.1f" );
            ImGui::SliderFloat3( "Saturation", modifiers.saturation, -1, 1, "%.1f" );
            ImGui::SliderFloat3( "Crosstalk", modifiers.crosstalk, 0.0f, 1.0f, "%.2f" );
            ImGui::TreePop();
        }
        if( ImGui::TreeNode( "Illumination" ) )
        {
            ImGui::Checkbox( "Anti-firefly", &devmode->antiFirefly );
            ImGui::SliderInt( "Shadow rays max depth",
                              &modifiers.maxBounceShadows,
                              0,
                              2,
                              "%d",
                              ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoInput );
            ImGui::Checkbox( "Second bounce for indirect",
                             &modifiers.enableSecondBounceForIndirect );
            ImGui::SliderFloat( "Sensitivity to change: Diffuse Direct",
                                &modifiers.directDiffuseSensitivityToChange,
                                0.0f,
                                1.0f,
                                "%.2f" );
            ImGui::SliderFloat( "Sensitivity to change: Diffuse Indirect",
                                &modifiers.indirectDiffuseSensitivityToChange,
                                0.0f,
                                1.0f,
                                "%.2f" );
            ImGui::SliderFloat( "Sensitivity to change: Specular",
                                &modifiers.specularSensitivityToChange,
                                0.0f,
                                1.0f,
                                "%.2f" );
            ImGui::TreePop();
        }
        if( ImGui::TreeNode( "Lightmap" ) )
        {
            ImGui::SliderFloat( "Screen coverage", &modifiers.lightmapScreenCoverage, 0.0f, 1.0f );
            ImGui::TreePop();
        }
        ImGui::EndDisabled();

        ImGui::Dummy( ImVec2( 0, 4 ) );
        ImGui::Separator();
        ImGui::Dummy( ImVec2( 0, 4 ) );

        if( ImGui::TreeNode( "Debug show" ) )
        {
            std::pair< const char*, uint32_t > fs[] = {
                { "Unfiltered diffuse direct", DEBUG_SHOW_FLAG_UNFILTERED_DIFFUSE },
                { "Unfiltered diffuse indirect", DEBUG_SHOW_FLAG_UNFILTERED_INDIRECT },
                { "Unfiltered specular", DEBUG_SHOW_FLAG_UNFILTERED_SPECULAR },
                { "Diffuse direct", DEBUG_SHOW_FLAG_ONLY_DIRECT_DIFFUSE },
                { "Diffuse indirect", DEBUG_SHOW_FLAG_ONLY_INDIRECT_DIFFUSE },
                { "Specular", DEBUG_SHOW_FLAG_ONLY_SPECULAR },
                { "Albedo white", DEBUG_SHOW_FLAG_ALBEDO_WHITE },
                { "Normals", DEBUG_SHOW_FLAG_NORMALS },
                { "Motion vectors", DEBUG_SHOW_FLAG_MOTION_VECTORS },
                { "Gradients", DEBUG_SHOW_FLAG_GRADIENTS },
                { "Light grid", DEBUG_SHOW_FLAG_LIGHT_GRID },
            };
            for( const auto [ name, f ] : fs )
            {
                ImGui::CheckboxFlags( name, &devmode->debugShowFlags, f );
            }
            ImGui::TreePop();
        }

        ImGui::Dummy( ImVec2( 0, 4 ) );
        ImGui::Separator();
        ImGui::Dummy( ImVec2( 0, 4 ) );

        if( ImGui::TreeNode( "Camera" ) )
        {
            auto& modifiers = devmode->cameraOvrd;

            ImGui::Checkbox( "FOV Override", &modifiers.fovEnable );
            {
                ImGui::BeginDisabled( !modifiers.fovEnable );
                ImGui::SliderFloat( "Vertical FOV", &modifiers.fovDeg, 10, 120, "%.0f degrees" );
                ImGui::EndDisabled();
            }

            ImGui::Checkbox( "Freelook", &modifiers.customEnable );
            ImGui::TextUnformatted(
                "Freelook:\n"
                "    * WASD - to move\n"
                "    * Alt - hold to rotate\n"
                "NOTE: inputs are read only from this window, and not from the game's one" );
            if( modifiers.customEnable )
            {
                if( ImGui::IsKeyPressed( ImGuiKey_LeftAlt ) )
                {
                    if( ImGui::IsMousePosValid() )
                    {
                        modifiers.intr_lastMouse  = { ImGui::GetMousePos().x,
                                                      ImGui::GetMousePos().y };
                        modifiers.intr_lastAngles = modifiers.customAngles;
                    }
                }
                if( ImGui::IsKeyReleased( ImGuiKey_LeftAlt ) )
                {
                    modifiers.intr_lastMouse  = {};
                    modifiers.intr_lastAngles = modifiers.customAngles;
                }

                if( modifiers.intr_lastMouse && ImGui::IsMousePosValid() )
                {
                    modifiers.customAngles = {
                        modifiers.intr_lastAngles.data[ 0 ] -
                            ( ImGui::GetMousePos().x - modifiers.intr_lastMouse->data[ 0 ] ),
                        modifiers.intr_lastAngles.data[ 1 ] -
                            ( ImGui::GetMousePos().y - modifiers.intr_lastMouse->data[ 1 ] ),
                    };
                }
                else
                {
                    modifiers.intr_lastMouse  = {};
                    modifiers.intr_lastAngles = modifiers.customAngles;
                }

                {
                    float speed = 0.1f * sceneImportExport->GetWorldScale();

                    RgFloat3D up, right;
                    Matrix::MakeUpRightFrom( up,
                                             right,
                                             Utils::DegToRad( modifiers.customAngles.data[ 0 ] ),
                                             Utils::DegToRad( modifiers.customAngles.data[ 1 ] ),
                                             sceneImportExport->GetWorldUp(),
                                             sceneImportExport->GetWorldRight() );
                    RgFloat3D fwd = Utils::Cross( up, right );

                    auto fma = []( const RgFloat3D& a, float mult, const RgFloat3D& b ) {
                        return RgFloat3D{ a.data[ 0 ] + mult * b.data[ 0 ],
                                          a.data[ 1 ] + mult * b.data[ 1 ],
                                          a.data[ 2 ] + mult * b.data[ 2 ] };
                    };

                    modifiers.customPos = fma(
                        modifiers.customPos, ImGui::IsKeyDown( ImGuiKey_A ) ? -speed : 0, right );
                    modifiers.customPos = fma(
                        modifiers.customPos, ImGui::IsKeyDown( ImGuiKey_D ) ? +speed : 0, right );
                    modifiers.customPos = fma(
                        modifiers.customPos, ImGui::IsKeyDown( ImGuiKey_W ) ? +speed : 0, fwd );
                    modifiers.customPos = fma(
                        modifiers.customPos, ImGui::IsKeyDown( ImGuiKey_S ) ? -speed : 0, fwd );
                }
            }
            ImGui::TreePop();
        }

        ImGui::Dummy( ImVec2( 0, 4 ) );
        ImGui::Separator();
        ImGui::Dummy( ImVec2( 0, 4 ) );
        devmode->breakOnTexture[ std::size( devmode->breakOnTexture ) - 1 ] = '\0';
        ImGui::TextUnformatted( "Debug break on texture: " );
        ImGui::Checkbox( "Image upload", &devmode->breakOnTextureImage );
        ImGui::Checkbox( "Primitive upload", &devmode->breakOnTexturePrimitive );
        ImGui::InputText( "##Debug break on texture text",
                          devmode->breakOnTexture,
                          std::size( devmode->breakOnTexture ) );

        ImGui::Dummy( ImVec2( 0, 4 ) );
        ImGui::Separator();
        ImGui::Dummy( ImVec2( 0, 4 ) );

        ImGui::Checkbox( "Always on top", &devmode->debugWindowOnTop );
        debugWindows->SetAlwaysOnTop( devmode->debugWindowOnTop );

        ImGui::Text( "%.3f ms/frame (%.1f FPS)",
                     1000.0f / ImGui::GetIO().Framerate,
                     ImGui::GetIO().Framerate );
        ImGui::EndTabItem();
    }

    if( ImGui::BeginTabItem( "Primitives" ) )
    {
        ImGui::Checkbox( "Ignore external geometry", &devmode->ignoreExternalGeometry );
        ImGui::Dummy( ImVec2( 0, 4 ) );
        ImGui::Separator();
        ImGui::Dummy( ImVec2( 0, 4 ) );

        using PrimMode = Devmode::DebugPrimMode;

        int*     modePtr = reinterpret_cast< int* >( &devmode->primitivesTableMode );
        PrimMode mode    = devmode->primitivesTableMode;

        ImGui::TextUnformatted( "Record: " );
        ImGui::SameLine();
        ImGui::RadioButton( "None", modePtr, static_cast< int >( PrimMode::None ) );
        ImGui::SameLine();
        ImGui::RadioButton( "Ray-traced", modePtr, static_cast< int >( PrimMode::RayTraced ) );
        ImGui::SameLine();
        ImGui::RadioButton( "Rasterized", modePtr, static_cast< int >( PrimMode::Rasterized ) );
        ImGui::SameLine();
        ImGui::RadioButton( "Non-world", modePtr, static_cast< int >( PrimMode::NonWorld ) );
        ImGui::SameLine();
        ImGui::RadioButton( "Decals", modePtr, static_cast< int >( PrimMode::Decal ) );

        ImGui::TextUnformatted(
            "Red    - if exportable, but not found in GLTF, so uploading as dynamic" );
        ImGui::TextUnformatted( "Green  - if exportable was found in GLTF" );

        if( ImGui::BeginTable( "Primitives table",
                               6,
                               ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable |
                                   ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti |
                                   ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                                   ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY ) )
        {
            {
                ImGui::TableSetupColumn( "Call",
                                         ImGuiTableColumnFlags_NoHeaderWidth |
                                             ImGuiTableColumnFlags_DefaultSort );
                ImGui::TableSetupColumn( "Object ID", ImGuiTableColumnFlags_NoHeaderWidth );
                ImGui::TableSetupColumn( "Mesh name", ImGuiTableColumnFlags_NoHeaderWidth );
                ImGui::TableSetupColumn( "Primitive index", ImGuiTableColumnFlags_NoHeaderWidth );
                ImGui::TableSetupColumn( "Primitive name", ImGuiTableColumnFlags_NoHeaderWidth );
                ImGui::TableSetupColumn( "Texture",
                                         ImGuiTableColumnFlags_NoHeaderWidth |
                                             ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableHeadersRow();
                if( ImGui::IsItemHovered() )
                {
                    ImGui::SetTooltip(
                        "Right-click to open menu\nMiddle-click to copy texture name" );
                }
            }

            if( ImGuiTableSortSpecs* sortspecs = ImGui::TableGetSortSpecs() )
            {
                sortspecs->SpecsDirty = true;

                std::ranges::sort(
                    devmode->primitivesTable,
                    [ sortspecs ]( const Devmode::DebugPrim& a,
                                   const Devmode::DebugPrim& b ) -> bool {
                        for( int n = 0; n < sortspecs->SpecsCount; n++ )
                        {
                            const ImGuiTableColumnSortSpecs* srt = &sortspecs->Specs[ n ];

                            std::strong_ordering ord{ 0 };
                            switch( srt->ColumnIndex )
                            {
                                case 0: ord = ( a.callIndex <=> b.callIndex ); break;
                                case 1: ord = ( a.objectId <=> b.objectId ); break;
                                case 2: ord = ( a.meshName <=> b.meshName ); break;
                                case 3: ord = ( a.primitiveIndex <=> b.primitiveIndex ); break;
                                case 4: ord = ( a.primitiveName <=> b.primitiveName ); break;
                                case 5: ord = ( a.textureName <=> b.textureName ); break;
                                default: assert( 0 ); return false;
                            }

                            if( std::is_gt( ord ) )
                            {
                                return srt->SortDirection != ImGuiSortDirection_Ascending;
                            }

                            if( std::is_lt( ord ) )
                            {
                                return srt->SortDirection == ImGuiSortDirection_Ascending;
                            }
                        }

                        return a.callIndex < b.callIndex;
                    } );
            }

            ImGuiListClipper clipper;
            clipper.Begin( int( devmode->primitivesTable.size() ) );
            while( clipper.Step() )
            {
                for( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                {
                    const auto& prim = devmode->primitivesTable[ i ];
                    ImGui::TableNextRow();

                    if( prim.result == UploadResult::ExportableStatic )
                    {
                        ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg0,
                                                IM_COL32( 0, 128, 0, 64 ) );
                        ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg1,
                                                IM_COL32( 0, 128, 0, 128 ) );
                    }
                    else if( prim.result == UploadResult::ExportableDynamic )
                    {
                        ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg0,
                                                IM_COL32( 128, 0, 0, 64 ) );
                        ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg1,
                                                IM_COL32( 128, 0, 0, 128 ) );
                    }
                    else
                    {
                        ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg0, IM_COL32( 0, 0, 0, 1 ) );
                        ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg1, IM_COL32( 0, 0, 0, 1 ) );
                    }


                    ImGui::TableNextColumn();
                    if( prim.result != UploadResult::Fail )
                    {
                        ImGui::Text( "%u", prim.callIndex );
                    }
                    else
                    {
                        ImGui::TextUnformatted( "fail" );
                    }

                    ImGui::TableNextColumn();
                    if( mode != PrimMode::Decal && mode != PrimMode::NonWorld )
                    {
                        ImGui::Text( "%llu", prim.objectId );
                    }

                    ImGui::TableNextColumn();
                    if( mode != PrimMode::Decal && mode != PrimMode::NonWorld )
                    {
                        ImGui::TextUnformatted( prim.meshName.c_str() );
                    }

                    ImGui::TableNextColumn();
                    if( mode != PrimMode::Decal )
                    {
                        ImGui::Text( "%u", prim.primitiveIndex );
                    }

                    ImGui::TableNextColumn();
                    if( mode != PrimMode::Decal )
                    {
                        ImGui::TextUnformatted( prim.primitiveName.c_str() );
                    }

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted( prim.textureName.c_str() );
                    if( ImGui::IsMouseReleased( ImGuiMouseButton_Middle ) &&
                        ImGui::IsItemHovered() )
                    {
                        ImGui::SetClipboardText( prim.textureName.c_str() );
                    }
                    else
                    {
                        if( ImGui::BeginPopupContextItem( std::format( "##popup{}", i ).c_str() ) )
                        {
                            if( ImGui::MenuItem( "Copy texture name" ) )
                            {
                                ImGui::SetClipboardText( prim.textureName.c_str() );
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::EndPopup();
                        }
                    }
                }
            }

            ImGui::EndTable();
        }
        ImGui::EndTabItem();
    }

    if( ImGui::BeginTabItem( "Log" ) )
    {
        ImGui::Checkbox( "Auto-scroll", &devmode->logAutoScroll );
        ImGui::SameLine();
        if( ImGui::Button( "Clear" ) )
        {
            devmode->logs.clear();
        }
        ImGui::Separator();

        ImGui::CheckboxFlags( "Errors", &devmode->logFlags, RG_MESSAGE_SEVERITY_ERROR );
        ImGui::SameLine();
        ImGui::CheckboxFlags( "Warnings", &devmode->logFlags, RG_MESSAGE_SEVERITY_WARNING );
        ImGui::SameLine();
        ImGui::CheckboxFlags( "Info", &devmode->logFlags, RG_MESSAGE_SEVERITY_INFO );
        ImGui::SameLine();
        ImGui::CheckboxFlags( "Verbose", &devmode->logFlags, RG_MESSAGE_SEVERITY_VERBOSE );
        ImGui::Separator();

        if( ImGui::BeginChild( "##LogScrollingRegion",
                               ImVec2( 0, 0 ),
                               false,
                               ImGuiWindowFlags_HorizontalScrollbar ) )
        {
            for( const auto& [ severity, count, text ] : devmode->logs )
            {
                RgMessageSeverityFlags filtered = severity & devmode->logFlags;

                if( filtered == 0 )
                {
                    continue;
                }

                std::optional< ImU32 > color;
                if( filtered & RG_MESSAGE_SEVERITY_ERROR )
                {
                    color = IM_COL32( 255, 0, 0, 255 );
                }
                else if( filtered & RG_MESSAGE_SEVERITY_WARNING )
                {
                    color = IM_COL32( 255, 255, 0, 255 );
                }

                if( color )
                {
                    ImGui::PushStyleColor( ImGuiCol_Text, *color );
                }

                if( count == 1 )
                {
                    ImGui::TextUnformatted( text.data() );
                }
                else
                {
                    ImGui::Text( "[%u] %s", count, text.data() );
                }

                if( color )
                {
                    ImGui::PopStyleColor();
                }
            }

            if( devmode->logAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
            {
                ImGui::SetScrollHereY( 1.0f );
            }
        }
        ImGui::EndChild();
        ImGui::EndTabItem();
    }

    if( ImGui::BeginTabItem( "Import/Export" ) )
    {
        auto& dev = sceneImportExport->dev;
        if( !dev.exportName.enable )
        {
            dev.exportName.SetDefaults( *sceneImportExport );
        }
        if( !dev.importName.enable )
        {
            dev.importName.SetDefaults( *sceneImportExport );
        }
        if( !dev.worldTransform.enable )
        {
            dev.worldTransform.SetDefaults( *sceneImportExport );
        }

        {
            ImGui::Text( "Resource folder: %s",
                         std::filesystem::absolute( ovrdFolder ).string().c_str() );
        }
        ImGui::Separator();
        ImGui::Dummy( ImVec2( 0, 16 ) );
        {
            ImGui::BeginDisabled( dev.buttonRecording );
            if( ImGui::Button( "Reimport replacements GLTF", { -1, 80 } ) )
            {
                sceneImportExport->RequestReplacementsReimport();
            }
            ImGui::Dummy( ImVec2( 0, 8 ) );
            if( ImGui::Button( "Reimport map GLTF", { -1, 80 } ) )
            {
                sceneImportExport->RequestReimport();
            }

            ImGui::Text( "Map import path: %s",
                         sceneImportExport->dev_GetSceneImportGltfPath().c_str() );
            ImGui::BeginDisabled( !dev.importName.enable );
            {
                ImGui::InputText(
                    "Import map name", dev.importName.value, std::size( dev.importName.value ) );
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::Checkbox( "Custom##import", &dev.importName.enable );
            ImGui::EndDisabled();
        }
        ImGui::Dummy( ImVec2( 0, 16 ) );
        ImGui::Separator();
        ImGui::Dummy( ImVec2( 0, 16 ) );
        {
            ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.98f, 0.59f, 0.26f, 0.40f ) );
            ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.98f, 0.59f, 0.26f, 1.00f ) );
            ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.98f, 0.53f, 0.06f, 1.00f ) );
            {
                const auto halfWidth = ( ImGui::GetContentRegionAvail().x -
                                         ImGui::GetCurrentWindow()->WindowPadding.x ) *
                                       0.5f;
                if( dev.buttonRecording )
                {
                    ImGui::BeginDisabled( true );
                    ImGui::Button( "Replacements are being recorded...", { halfWidth, 80 } );
                    ImGui::EndDisabled();
                }
                else
                {
                    if( ImGui::Button( "Export replacements GLTF\n from this frame",
                                       { halfWidth, 80 } ) )
                    {
                        sceneImportExport->RequestReplacementsExport_OneFrame();
                    }
                }
                ImGui::SameLine();
                if( dev.buttonRecording )
                {
                    if( ImGui::Button( "Stop recording\nand Export into GLTF", { halfWidth, 80 } ) )
                    {
                        sceneImportExport->RequestReplacementsExport_RecordEnd();
                        dev.buttonRecording = false;
                    }
                }
                else
                {
                    if( ImGui::Button( "Start recording\nreplacements into GLTF",
                                       { halfWidth, 80 } ) )
                    {
                        sceneImportExport->RequestReplacementsExport_RecordBegin();
                        dev.buttonRecording = true;
                    }
                }
            }
            ImGui::BeginDisabled( dev.buttonRecording );
            ImGui::Checkbox( "Allow export of existing replacements",
                             &devmode->allowExportOfExistingReplacements );
            ImGui::Dummy( ImVec2( 0, 16 ) );
            if( ImGui::Button( "Export map GLTF", { -1, 80 } ) )
            {
                sceneImportExport->RequestExport();
            }
            ImGui::PopStyleColor( 3 );

            ImGui::Text( "Export path: %s",
                         sceneImportExport->dev_GetSceneExportGltfPath().c_str() );
            ImGui::BeginDisabled( !dev.exportName.enable );
            {
                ImGui::InputText(
                    "Export map name", dev.exportName.value, std::size( dev.exportName.value ) );
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::Checkbox( "Custom##export", &dev.exportName.enable );
            ImGui::EndDisabled();
        }
        ImGui::Dummy( ImVec2( 0, 16 ) );
        ImGui::Separator();
        ImGui::Dummy( ImVec2( 0, 16 ) );
        {
            ImGui::BeginDisabled( dev.buttonRecording );
            ImGui::Checkbox( "Custom import/export world space", &dev.worldTransform.enable );
            ImGui::BeginDisabled( !dev.worldTransform.enable );
            {
                ImGui::SliderFloat3( "World Up vector", dev.worldTransform.up.data, -1.0f, 1.0f );
                ImGui::SliderFloat3(
                    "World Forward vector", dev.worldTransform.forward.data, -1.0f, 1.0f );
                ImGui::InputFloat(
                    std::format( "1 unit = {} meters", dev.worldTransform.scale ).c_str(),
                    &dev.worldTransform.scale );
            }
            ImGui::EndDisabled();
            ImGui::EndDisabled();
        }
        ImGui::EndTabItem();
    }

    if( ImGui::BeginTabItem( "Textures" ) )
    {
        if( ImGui::Button( "Export original textures", { -1, 80 } ) )
        {
            textureManager->ExportOriginalMaterialTextures( ovrdFolder /
                                                            TEXTURES_FOLDER_ORIGINALS );
        }
        ImGui::Text( "Export path: %s",
                     ( ovrdFolder / TEXTURES_FOLDER_ORIGINALS ).string().c_str() );
        ImGui::Dummy( ImVec2( 0, 16 ) );
        ImGui::Separator();
        ImGui::Dummy( ImVec2( 0, 16 ) );

        enum
        {
            ColumnTextureIndex0,
            ColumnTextureIndex1,
            ColumnTextureIndex2,
            ColumnTextureIndex3,
            ColumnMaterialName,
            Column_Count,
        };
        static_assert( std::size( TextureManager::Debug_MaterialInfo{}.textures.indices ) == 4 );

        ImGui::Checkbox( "Record", &devmode->materialsTableEnable );
        ImGui::TextUnformatted( "Blue - if material is non-original (i.e. was loaded from GLTF)" );
        if( ImGui::BeginTable( "Materials table",
                               Column_Count,
                               ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable |
                                   ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti |
                                   ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                                   ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY ) )
        {
            auto materialInfos = devmode->materialsTableEnable
                                     ? textureManager->Debug_GetMaterials()
                                     : std::vector< TextureManager::Debug_MaterialInfo >{};
            {
                ImGui::TableSetupColumn( "A", 0, 8 );
                ImGui::TableSetupColumn( "P", 0, 8 );
                ImGui::TableSetupColumn( "N", 0, 8 );
                ImGui::TableSetupColumn( "E", 0, 8 );
                ImGui::TableSetupColumn( "Material name",
                                         ImGuiTableColumnFlags_WidthStretch |
                                             ImGuiTableColumnFlags_DefaultSort,
                                         -1 );
                ImGui::TableHeadersRow();
                if( ImGui::IsItemHovered() )
                {
                    ImGui::SetTooltip(
                        "Right-click to open menu\nMiddle-click to copy texture name" );
                }
            }

            if( ImGuiTableSortSpecs* sortspecs = ImGui::TableGetSortSpecs() )
            {
                sortspecs->SpecsDirty = true;

                std::ranges::sort(
                    materialInfos,
                    [ sortspecs ]( const TextureManager::Debug_MaterialInfo& a,
                                   const TextureManager::Debug_MaterialInfo& b ) -> bool {
                        for( int n = 0; n < sortspecs->SpecsCount; n++ )
                        {
                            const ImGuiTableColumnSortSpecs* srt = &sortspecs->Specs[ n ];

                            std::strong_ordering ord{ 0 };
                            switch( srt->ColumnIndex )
                            {
                                case ColumnTextureIndex0:
                                    ord = ( a.textures.indices[ 0 ] <=> b.textures.indices[ 0 ] );
                                    break;
                                case ColumnTextureIndex1:
                                    ord = ( a.textures.indices[ 1 ] <=> b.textures.indices[ 1 ] );
                                    break;
                                case ColumnTextureIndex2:
                                    ord = ( a.textures.indices[ 2 ] <=> b.textures.indices[ 2 ] );
                                    break;
                                case ColumnTextureIndex3:
                                    ord = ( a.textures.indices[ 3 ] <=> b.textures.indices[ 3 ] );
                                    break;
                                case ColumnMaterialName:
                                    ord = ( a.materialName <=> b.materialName );
                                    break;
                                default: continue;
                            }

                            if( std::is_gt( ord ) )
                            {
                                return srt->SortDirection != ImGuiSortDirection_Ascending;
                            }

                            if( std::is_lt( ord ) )
                            {
                                return srt->SortDirection == ImGuiSortDirection_Ascending;
                            }
                        }

                        return a.materialName < b.materialName;
                    } );
            }

            ImGuiListClipper clipper;
            clipper.Begin( int( materialInfos.size() ) );
            while( clipper.Step() )
            {
                for( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                {
                    const auto& mat = materialInfos[ i ];
                    ImGui::TableNextRow();
                    ImGui::PushID( i );

                    if( mat.isOriginal )
                    {
                        ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg0,
                                                IM_COL32( 0, 0, 128, 64 ) );
                        ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg1,
                                                IM_COL32( 0, 0, 128, 128 ) );
                    }
                    else
                    {
                        ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg0, IM_COL32( 0, 0, 0, 1 ) );
                        ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg1, IM_COL32( 0, 0, 0, 1 ) );
                    }

                    auto writeTexIndex = [ &mat ]( int channel ) {
                        assert( channel >= 0 && channel < std::size( mat.textures.indices ) );
                        if( mat.textures.indices[ channel ] != EMPTY_TEXTURE_INDEX )
                        {
                            ImGui::Text( "%u", mat.textures.indices[ channel ] );
                        }
                    };

                    for( auto col = 0; col < Column_Count; col++ )
                    {
                        ImGui::TableNextColumn();

                        switch( col )
                        {
                            case ColumnTextureIndex0:
                                writeTexIndex( 0 );
                                if( ImGui::TableGetColumnFlags( col ) &
                                    ImGuiTableColumnFlags_IsHovered )
                                {
                                    ImGui::SetTooltip( "Image\n[RGB]Albedo\n[A] "
                                                       "Alpha (0.0 - fully transparent)" );
                                }
                                break;

                            case ColumnTextureIndex1:
                                writeTexIndex( 1 );
                                if( ImGui::TableGetColumnFlags( col ) &
                                    ImGuiTableColumnFlags_IsHovered )
                                {
                                    ImGui::SetTooltip(
                                        "Image\n[R]Occlusion (disabled by default)\n[G] "
                                        "Roughness\n[B] Metallic" );
                                }
                                break;

                            case ColumnTextureIndex2:
                                writeTexIndex( 2 );
                                if( ImGui::TableGetColumnFlags( col ) &
                                    ImGuiTableColumnFlags_IsHovered )
                                {
                                    ImGui::SetTooltip(
                                        "Image\n[R] Normal X offset\n[G] Normal Y offset" );
                                }
                                break;

                            case ColumnTextureIndex3:
                                writeTexIndex( 3 );
                                if( ImGui::TableGetColumnFlags( col ) &
                                    ImGuiTableColumnFlags_IsHovered )
                                {
                                    ImGui::SetTooltip( "Image\n[RGB] Emission color" );
                                }
                                break;

                            case ColumnMaterialName:
                                ImGui::TextUnformatted( mat.materialName.c_str() );

                                if( ImGui::IsMouseReleased( ImGuiMouseButton_Middle ) &&
                                    ImGui::IsItemHovered() )
                                {
                                    ImGui::SetClipboardText( mat.materialName.c_str() );
                                }
                                else
                                {
                                    if( ImGui::BeginPopupContextItem(
                                            std::format( "##popup{}", i ).c_str() ) )
                                    {
                                        if( ImGui::MenuItem( "Copy texture name" ) )
                                        {
                                            ImGui::SetClipboardText( mat.materialName.c_str() );
                                            ImGui::CloseCurrentPopup();
                                        }
                                        ImGui::EndPopup();
                                    }
                                }
                                break;

                            default: break;
                        }
                    }

                    ImGui::PopID();
                }
            }

            ImGui::EndTable();
        }
        ImGui::EndTabItem();
    }
}

void RTGL1::VulkanDevice::Dev_Override( RgStartFrameInfo& info ) const
{
    if( !Dev_IsDevmodeInitialized() )
    {
        return;
    }

    auto& modifiers = devmode->drawInfoOvrd;

    if( modifiers.enable )
    {
        RgStartFrameInfo& dst = info;

        // apply modifiers
        {
            dst.vsync = modifiers.vsync;
        }
    }
    else
    {
        const RgStartFrameInfo& src = info;

        // reset modifiers
        {
            modifiers.vsync = src.vsync;
        }
    }
}

void RTGL1::VulkanDevice::Dev_Override( RgCameraInfo& info ) const
{
    if( !Dev_IsDevmodeInitialized() )
    {
        assert( 0 );
        return;
    }

    auto& modifiers = devmode->cameraOvrd;

    if( modifiers.fovEnable )
    {
        info.fovYRadians = Utils::DegToRad( modifiers.fovDeg );
    }
    else
    {
        modifiers.fovDeg = Utils::RadToDeg( info.fovYRadians );
    }

    if( modifiers.customEnable )
    {
        RgCameraInfo& dst_camera = info;

        dst_camera.position = modifiers.customPos;
        Matrix::MakeUpRightFrom( dst_camera.up,
                                 dst_camera.right,
                                 Utils::DegToRad( modifiers.customAngles.data[ 0 ] ),
                                 Utils::DegToRad( modifiers.customAngles.data[ 1 ] ),
                                 sceneImportExport->GetWorldUp(),
                                 sceneImportExport->GetWorldRight() );
    }
    else
    {
        const RgCameraInfo& src_camera = info;

        modifiers.customPos    = src_camera.position;
        modifiers.customAngles = { 0, 0 };
    }
}

void RTGL1::VulkanDevice::Dev_Override( RgDrawFrameRenderResolutionParams& resolution,
                                        RgDrawFrameIlluminationParams&     illumination,
                                        RgDrawFrameTonemappingParams&      tonemappingp,
                                        RgDrawFrameLightmapParams&         lightmap ) const
{
    if( !Dev_IsDevmodeInitialized() )
    {
        return;
    }

    auto& modifiers = devmode->drawInfoOvrd;

    if( modifiers.enable )
    {
        RgDrawFrameRenderResolutionParams& dst_resol = resolution;
        RgDrawFrameIlluminationParams&     dst_illum = illumination;
        RgDrawFrameTonemappingParams&      dst_tnmp  = tonemappingp;
        RgDrawFrameLightmapParams&         dst_ltmp  = lightmap;

        // apply modifiers
        {
            float aspect = float( renderResolution.UpscaledWidth() ) /
                           float( renderResolution.UpscaledHeight() );

            dst_resol.upscaleTechnique = modifiers.upscaleTechnique;
            dst_resol.sharpenTechnique = modifiers.sharpenTechnique;
            dst_resol.resolutionMode   = modifiers.resolutionMode;
            dst_resol.customRenderSize = {
                ClampPix< uint32_t >( modifiers.customRenderSizeScale *
                                      float( renderResolution.UpscaledWidth() ) ),
                ClampPix< uint32_t >( modifiers.customRenderSizeScale *
                                      float( renderResolution.UpscaledHeight() ) ),
            };
            dst_resol.pixelizedRenderSizeEnable = modifiers.pixelizedEnable;
            dst_resol.pixelizedRenderSize       = {
                ClampPix< uint32_t >(
                    static_cast< uint32_t >( aspect * float( modifiers.pixelizedHeight ) ) ),
                ClampPix< uint32_t >( modifiers.pixelizedHeight ),
            };
        }
        {
            dst_illum.maxBounceShadows                 = modifiers.maxBounceShadows;
            dst_illum.enableSecondBounceForIndirect    = modifiers.enableSecondBounceForIndirect;
            dst_illum.directDiffuseSensitivityToChange = modifiers.directDiffuseSensitivityToChange;
            dst_illum.indirectDiffuseSensitivityToChange =
                modifiers.indirectDiffuseSensitivityToChange;
            dst_illum.specularSensitivityToChange = modifiers.specularSensitivityToChange;
        }
        {
            dst_tnmp.disableEyeAdaptation = modifiers.disableEyeAdaptation;
            dst_tnmp.ev100Min             = modifiers.ev100Min;
            dst_tnmp.ev100Max             = modifiers.ev100Max;
            dst_tnmp.saturation           = { RG_ACCESS_VEC3( modifiers.saturation ) };
            dst_tnmp.crosstalk            = { RG_ACCESS_VEC3( modifiers.crosstalk ) };
        }
        {
            dst_ltmp.lightmapScreenCoverage = modifiers.lightmapScreenCoverage;
        }
    }
    else
    {
        const RgDrawFrameRenderResolutionParams& src_resol = resolution;
        const RgDrawFrameIlluminationParams&     src_illum = illumination;
        const RgDrawFrameTonemappingParams&      src_tnmp  = tonemappingp;
        const RgDrawFrameLightmapParams&         src_ltmp  = lightmap;

        // reset modifiers
        {
            devmode->antiFirefly = true;
        }
        {
            modifiers.upscaleTechnique = src_resol.upscaleTechnique;
            modifiers.sharpenTechnique = src_resol.sharpenTechnique;
            modifiers.resolutionMode   = src_resol.resolutionMode;

            if( modifiers.resolutionMode == RG_RENDER_RESOLUTION_MODE_CUSTOM )
            {
                modifiers.customRenderSizeScale = float( src_resol.customRenderSize.height ) /
                                                  float( renderResolution.UpscaledHeight() );
            }
            else
            {
                modifiers.customRenderSizeScale = 1.0f;
            }

            modifiers.pixelizedEnable = src_resol.pixelizedRenderSizeEnable;
            modifiers.pixelizedHeight =
                src_resol.pixelizedRenderSizeEnable
                    ? ClampPix< int >( src_resol.pixelizedRenderSize.height )
                    : 0;
        }
        {
            modifiers.maxBounceShadows                 = int( src_illum.maxBounceShadows );
            modifiers.enableSecondBounceForIndirect    = src_illum.enableSecondBounceForIndirect;
            modifiers.directDiffuseSensitivityToChange = src_illum.directDiffuseSensitivityToChange;
            modifiers.indirectDiffuseSensitivityToChange =
                src_illum.indirectDiffuseSensitivityToChange;
            modifiers.specularSensitivityToChange = src_illum.specularSensitivityToChange;
        }
        {
            modifiers.disableEyeAdaptation = src_tnmp.disableEyeAdaptation;
            modifiers.ev100Min             = src_tnmp.ev100Min;
            modifiers.ev100Max             = src_tnmp.ev100Max;
            RG_SET_VEC3_A( modifiers.saturation, src_tnmp.saturation.data );
            RG_SET_VEC3_A( modifiers.crosstalk, src_tnmp.crosstalk.data );
        }
        {
            modifiers.lightmapScreenCoverage = src_ltmp.lightmapScreenCoverage;
        }
    }
}

void RTGL1::VulkanDevice::Dev_TryBreak( const char* pTextureName, bool isImageUpload )
{
#ifdef _MSC_VER
    if( !devmode )
    {
        return;
    }

    if( isImageUpload )
    {
        if( !devmode->breakOnTextureImage )
        {
            return;
        }
    }
    else
    {
        if( !devmode->breakOnTexturePrimitive )
        {
            return;
        }
    }

    if( Utils::IsCstrEmpty( devmode->breakOnTexture ) || Utils::IsCstrEmpty( pTextureName ) )
    {
        return;
    }

    devmode->breakOnTexture[ std::size( devmode->breakOnTexture ) - 1 ] = '\0';
    if( std::strcmp( devmode->breakOnTexture, Utils::SafeCstr( pTextureName ) ) == 0 )
    {
        __debugbreak();
        devmode->breakOnTextureImage     = false;
        devmode->breakOnTexturePrimitive = false;
    }
#endif
}
