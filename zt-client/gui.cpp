#include "include/gui.hpp"
#include "include/clay.h"
#include "raylib.h"
#include "raymath.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

// --- Utilities ---

std::string formatSize(uint64_t bytes) {
    const char* suffixes[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int i = 0;
    double dblBytes = bytes;
    if (bytes > 1024) {
        for (i = 0; (bytes / 1024) > 0 && i < 5; i++, bytes /= 1024)
            dblBytes = bytes / 1024.0;
    }
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << dblBytes << " " << suffixes[i];
    return ss.str();
}

// --- Clay Impl & Renderer ---

#define CLAY_IMPLEMENTATION
#include "include/clay.h"

// Basic Raylib Renderer for Clay adaptation
// Based on official example but simplified

Clay_Dimensions Raylib_MeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData) {
    (void)userData;
    Font font = GetFontDefault(); // Using default font for simplicity
    float fontSize = (float)config->fontSize;
    float spacing = (float)config->letterSpacing;
    
    // Copy to null-terminated string
    std::string str(text.chars, text.length);
    Vector2 size = MeasureTextEx(font, str.c_str(), fontSize, spacing);
    
    return { size.x, size.y };
}

void Clay_Raylib_Render(Clay_RenderCommandArray renderCommands) {
    for (int j = 0; j < renderCommands.length; j++) {
        Clay_RenderCommand *renderCommand = Clay_RenderCommandArray_Get(&renderCommands, j);
        Rectangle bbox = { 
            renderCommand->boundingBox.x, 
            renderCommand->boundingBox.y, 
            renderCommand->boundingBox.width, 
            renderCommand->boundingBox.height 
        };

        switch (renderCommand->commandType) {
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
                Clay_RectangleRenderData *config = &renderCommand->renderData.rectangle;
                Color color = { 
                    (unsigned char)roundf(config->backgroundColor.r), 
                    (unsigned char)roundf(config->backgroundColor.g), 
                    (unsigned char)roundf(config->backgroundColor.b), 
                    (unsigned char)roundf(config->backgroundColor.a) 
                };
                
                if (config->cornerRadius.topLeft > 0) {
                    float radius = config->cornerRadius.topLeft;
                    DrawRectangleRounded(bbox, radius / (float)((bbox.width > bbox.height) ? bbox.height : bbox.width), 8, color);
                } else {
                    DrawRectangleRec(bbox, color);
                }
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_TEXT: {
                Clay_TextRenderData *textData = &renderCommand->renderData.text;
                std::string text(textData->stringContents.chars, textData->stringContents.length);
                Color color = { 
                    (unsigned char)roundf(textData->textColor.r), 
                    (unsigned char)roundf(textData->textColor.g), 
                    (unsigned char)roundf(textData->textColor.b), 
                    (unsigned char)roundf(textData->textColor.a) 
                };
                Font font = GetFontDefault();
                DrawTextEx(font, text.c_str(), {bbox.x, bbox.y}, (float)textData->fontSize, (float)textData->letterSpacing, color);
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
                BeginScissorMode((int)bbox.x, (int)bbox.y, (int)bbox.width, (int)bbox.height);
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
                EndScissorMode();
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_BORDER: {
                 Clay_BorderRenderData *config = &renderCommand->renderData.border;
                 Color color = { 
                    (unsigned char)roundf(config->color.r), 
                    (unsigned char)roundf(config->color.g), 
                    (unsigned char)roundf(config->color.b), 
                    (unsigned char)roundf(config->color.a) 
                };
                
                if (config->width.left > 0) DrawRectangleRec({bbox.x, bbox.y, (float)config->width.left, bbox.height}, color);
                if (config->width.right > 0) DrawRectangleRec({bbox.x + bbox.width - config->width.right, bbox.y, (float)config->width.right, bbox.height}, color);
                if (config->width.top > 0) DrawRectangleRec({bbox.x, bbox.y, bbox.width, (float)config->width.top}, color);
                if (config->width.bottom > 0) DrawRectangleRec({bbox.x, bbox.y + bbox.height - config->width.bottom, bbox.width, (float)config->width.bottom}, color);
                break;
            }
            default: break;
        }
    }
}

void HandleClayErrors(Clay_ErrorData errorData) {
    std::cerr << "Clay Error: " << errorData.errorText.chars << std::endl;
}


// --- GUI Application ---

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 800;

// Theme Colors
const Clay_Color COLOR_BACKGROUND = {20, 20, 25, 255};      // Dark Blue/Grey
const Clay_Color COLOR_PANEL = {30, 32, 40, 255};           // Slightly lighter
const Clay_Color COLOR_ACCENT = {64, 164, 255, 255};        // Bright Blue
const Clay_Color COLOR_DANGER = {220, 60, 60, 255};         // Red
const Clay_Color COLOR_TEXT_PRIMARY = {240, 240, 240, 255}; // White
const Clay_Color COLOR_TEXT_SECONDARY = {160, 160, 170, 255};// Grey

// Interaction state
struct AppState {
    std::vector<Device> devices;
    float scrollY = 0;
};

void RenderDeviceCard(const Device& dev) {
    CLAY(
        CLAY_LAYOUT({ 
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
            .padding = { 16, 16 },
            .childGap = 16,
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        }),
        CLAY_RECTANGLE({ 
            .color = COLOR_PANEL, 
            .cornerRadius = { 8, 8, 8, 8 } 
        }),
        CLAY_BORDER({ // Border to separate
             .color = {50, 50, 60, 255},
             .width = {1, 1, 1, 1},
             .cornerRadius = {8, 8, 8, 8}
        })
    ) {
        // Top Row: Name and Type
        CLAY(CLAY_LAYOUT({ 
            .sizing = { .width = CLAY_SIZING_GROW(0) },
            .childGap = 10,
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .gaps = { .rowGap = 0, } // rowGap not valid in gaps for some versions, but childGap works
        })) {
             // Icon or Type Badge
            CLAY(
                CLAY_LAYOUT({ .padding = {8, 8} }),
                CLAY_RECTANGLE({ .color = COLOR_ACCENT, .cornerRadius = {4,4,4,4} })
            ) {
                 Clay_StringSlice typeStr = { (int)dev.type.length(), dev.type.data() };
                 CLAY_TEXT(typeStr, CLAY_TEXT_CONFIG({ .fontSize = 16, .textColor = {255,255,255,255} }));
            }

            // Name
            CLAY(CLAY_LAYOUT({ .padding = {6, 0} })) {
                Clay_StringSlice nameStr = { (int)dev.name.length(), dev.name.data() };
                CLAY_TEXT(nameStr, CLAY_TEXT_CONFIG({ .fontSize = 20, .textColor = COLOR_TEXT_PRIMARY }));
            }
        }

        // Details Row
        CLAY(CLAY_LAYOUT({ .layoutDirection = CLAY_LEFT_TO_RIGHT, .childGap = 20 })) {
            // Model
            std::string modelText = "Model: " + (dev.model.empty() ? "Unknown" : dev.model);
            CLAY_TEXT(Clay_StringSlice({(int)modelText.length(), modelText.data()}), 
                      CLAY_TEXT_CONFIG({ .fontSize = 16, .textColor = COLOR_TEXT_SECONDARY }));
            
            // Size
            std::string sizeText = "Size: " + formatSize(dev.sizeBytes);
            CLAY_TEXT(Clay_StringSlice({(int)sizeText.length(), sizeText.data()}), 
                      CLAY_TEXT_CONFIG({ .fontSize = 16, .textColor = COLOR_TEXT_SECONDARY }));
            
            // Flags
            if (dev.isReadOnly) {
                CLAY_TEXT(CLAY_STRING(" [READ-ONLY] "), CLAY_TEXT_CONFIG({ .fontSize = 14, .textColor = {255, 100, 100, 255} }));
            }
            if (dev.isRemovable) {
                CLAY_TEXT(CLAY_STRING(" [REMOVABLE] "), CLAY_TEXT_CONFIG({ .fontSize = 14, .textColor = {100, 255, 100, 255} }));
            }
        }

        // Actions Row
        CLAY(CLAY_LAYOUT({ 
            .sizing = { .width = CLAY_SIZING_GROW(0) },
            .childGap = 10,
            .layoutDirection = CLAY_LEFT_TO_RIGHT
        })) {
             CLAY(
                CLAY_LAYOUT({ .padding = {12, 12}, .childGap = 8 }),
                CLAY_RECTANGLE({ .color = COLOR_DANGER, .cornerRadius = {4,4,4,4} })
            ) {
                 CLAY_TEXT(CLAY_STRING("Wipe Drive"), CLAY_TEXT_CONFIG({ .fontSize = 16, .textColor = {255,255,255,255} }));
            }
        }
    }
}


void runGui() {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "ZeroTrace - Secure Erase Utility");
    SetTargetFPS(60);
    
    // Check Clay
    uint64_t clayMemorySize = Clay_MinMemorySize();
    Clay_Arena clayArena = Clay_CreateArenaWithCapacityAndMemory(clayMemorySize, (char*)malloc(clayMemorySize));
    Clay_Initialize(clayArena, {WINDOW_WIDTH, WINDOW_HEIGHT}, {HandleClayErrors});
    Clay_SetMeasureTextFunction(Raylib_MeasureText);

    AppState appState;
    std::cout << "Scanning for devices..." << std::endl;
    appState.devices = getDevices();
    std::cout << "Found " << appState.devices.size() << " devices." << std::endl;

    // Load a font if needed, otherwise default is used.
    
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Vector2 mousePos = GetMousePosition();
        bool isLeftClick = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        float wheelMove = GetMouseWheelMove();

        // Scroll logic (simple)
        appState.scrollY += wheelMove * 20.0f;
        if (appState.scrollY > 0) appState.scrollY = 0; // limit top

        Clay_SetPointerState({mousePos.x, mousePos.y}, isLeftClick);
        Clay_UpdateScrollContainers(true, {0, wheelMove * 20}, dt);

        Clay_BeginLayout();

        // Main Container
        CLAY(
            CLAY_ID("MainContainer"),
            CLAY_LAYOUT({ 
                .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .padding = {0,0},
                .childGap = 0
            }),
            CLAY_RECTANGLE({ .color = COLOR_BACKGROUND })
        ) {
            // Header
            CLAY(
                CLAY_LAYOUT({ 
                    .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(80) },
                    .padding = { 32, 20 },
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    .childGap = 20
                }),
                CLAY_RECTANGLE({ .color = {25, 25, 30, 255} })
            ) {
                CLAY_TEXT(CLAY_STRING("ZeroTrace"), CLAY_TEXT_CONFIG({ .fontSize = 32, .textColor = COLOR_ACCENT }));
                
                CLAY_TEXT(CLAY_STRING("|  Secure Data Cleanup"), CLAY_TEXT_CONFIG({ .fontSize = 24, .textColor = COLOR_TEXT_SECONDARY }));
            
                // Refresh Button (Fake)
                CLAY(
                    CLAY_LAYOUT({ .padding = {10,10} }),
                    CLAY_RECTANGLE({ .color = {50,50,60,255}, .cornerRadius = {4,4,4,4} })
                ) {
                      CLAY_TEXT(CLAY_STRING("Refresh"), CLAY_TEXT_CONFIG({ .fontSize = 18, .textColor = COLOR_TEXT_PRIMARY }));
                }
            }

            // Content Area (Scrollable)
            CLAY(
                CLAY_ID("ContentArea"),
                CLAY_LAYOUT({ 
                    .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
                    .padding = { 32, 32 },
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .childGap = 16
                })
            ) {
                 CLAY_TEXT(CLAY_STRING("Connected Storage Devices"), CLAY_TEXT_CONFIG({ .fontSize = 24, .textColor = COLOR_TEXT_PRIMARY }));
                 
                 if (appState.devices.empty()) {
                     CLAY(CLAY_LAYOUT({ .padding = {20, 20} })) {
                         CLAY_TEXT(CLAY_STRING("No devices found (run as sudo?)"), CLAY_TEXT_CONFIG({ .fontSize = 20, .textColor = COLOR_DANGER }));
                     }
                 } else {
                     for (const auto& dev : appState.devices) {
                         RenderDeviceCard(dev);
                     }
                 }
            }
        }

        Clay_RenderCommandArray renderCommands = Clay_EndLayout();

        BeginDrawing();
        ClearBackground(CLAY_COLOR_TO_RAYLIB_COLOR(COLOR_BACKGROUND));
        Clay_Raylib_Render(renderCommands);
        EndDrawing();
    }

    CloseWindow();
}
