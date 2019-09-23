#include <SDL2/SDL.h> 
#include <iostream>

#include "renderer.h"
#include "main.h"
#include "common.h"
#include "SDL_ttf.h"
#include <stdio.h>
#include <sstream> 
#include <iomanip>

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 540

bool gIsRunning = true;
float gAverageFPS = 0.0f;
SDL_Window* gWindow = NULL;
unsigned gWindowID = 0;
TTF_Font* gFont = nullptr;

float CalcAverageTick(float newtick);

std::string GetSourceDirPath()
{
    std::string dirPath = __FILE__;
    int pos = dirPath.find("main.cpp");
    dirPath = dirPath.substr(0, pos);
    return dirPath;
}

bool InitFonts()
{
    //Initialize SDL_ttf
    if (TTF_Init() == -1)
    {
        printf("SDL could not initialize SDL TTF!");
        return false;
    }

    std::string fontPath = GetSourceDirPath() + "Roboto-Medium.ttf";
    gFont = TTF_OpenFont(fontPath.c_str(), 18);
    if (gFont == NULL)
    {
        printf("%s", TTF_GetError());
        return false;
    }

    return true;
}

bool Init()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    gWindow = SDL_CreateWindow("PGR", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (gWindow == NULL)
    {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    gWindowID = SDL_GetWindowID(gWindow);
  
    return InitFonts();
}

void RenderText(const char *text, SDL_Color color, SDL_Rect textRect, SDL_Surface *destSurface)
{
    SDL_Surface *textSurface = TTF_RenderText_Solid(gFont, text, color);
    SDL_BlitSurface(textSurface, &textRect, destSurface, NULL);
    SDL_FreeSurface(textSurface);
}

void RenderScreen(RenderContext *context, double dt)
{
    // Create SDL_Surface from our framebuffer
    SDL_Surface *pixelSurface = SDL_CreateRGBSurfaceFrom(context->rasterizer.frameBuffer,
                                                         context->width,
                                                         context->height,
                                                         8 * 4,										   // depth in bits (BitsPerByte * BytesPerPixel)
                                                         context->width * 4,						   // pitch (row length * BytesPerPixel)
                                                         0x000000ff,                                   // red mask
                                                         0x0000ff00,                                   // green mask
                                                         0x00ff0000,                                   // blue mask
                                                         0);                                           // alpha mask

    SDL_Color color = SDL_Color{ 200, 200, 200 };
    SDL_Rect textRect = {};
    textRect.w = context->width;
    textRect.x = 0;
    textRect.y = 0;
    textRect.h = context->height;

    std::stringstream ss;
    ss << "  Shading: " << Renderer::ShadingToString(context->shading) << " (1)";
    RenderText(ss.str().c_str(), color, textRect, pixelSurface);

    textRect.x = -context->width + 150;
    textRect.y = -5;
    textRect.h = context->height;
    std::stringstream ss7;
    ss7 << " (S) Secret: " <<  (context->solarSystem ? "on" : "off");
    RenderText(ss7.str().c_str(), color, textRect, pixelSurface);

    textRect.x = 0;
    textRect.y = -25;
    std::stringstream ss2;	
    ss2 << "  Shininess: " << context->shininess << " (2)(3)"; 
    RenderText(ss2.str().c_str(), color, textRect, pixelSurface);

    textRect.x = 0;
    textRect.y = -50;
    unsigned numSphereTris = (context->sphereSubdivisions * 2 + ((context->sphereSubdivisions - 2) * context->sphereSubdivisions * 2));
    std::stringstream ss4;
    ss4 << "  Sphere Triangles: " << numSphereTris << " (4)(5)";
    RenderText(ss4.str().c_str(), color, textRect, pixelSurface);

    // Blit (copy) it to the window
    SDL_BlitSurface(pixelSurface, NULL, SDL_GetWindowSurface(gWindow), NULL);

    SDL_FreeSurface(pixelSurface);
}

void onWindowResized(int w, int h, RenderContext *context)
{
    if (w > 50 && h > 50)
    {
        context->width = w;
        context->height = h;
    }
}

void onMouseMove(SDL_Event &event, RenderContext *context)
{
    if (event.window.windowID == gWindowID && (event.motion.state & SDL_BUTTON_LMASK))
    {
        context->mouseRelX += event.motion.xrel;
        context->mouseRelY += event.motion.yrel;
    }
}

void onMouseDown(Uint8 button, unsigned /*x*/, unsigned /*y*/)
{
    if (button == SDL_BUTTON_LEFT)
    {
        SDL_ShowCursor(SDL_DISABLE);
    }
}

void onMouseWheel(Sint32 y, RenderContext *context)
{
    context->mouseWheel += y;
}

void onMouseUp(Uint8 button, unsigned /*x*/, unsigned /*y*/)
{
    if (button == SDL_BUTTON_LEFT)
    {
        SDL_ShowCursor(SDL_ENABLE);
    }
}

void onKeyDown(SDL_Keycode key, RenderContext *context)
{
    switch (key) 
    {
        case SDLK_ESCAPE: gIsRunning = false; return;
        case SDLK_UP:
            break;
        case SDLK_DOWN:
            break;
        case SDLK_1:
            switch (context->shading)
            {
                case FLAT_SHADING:
                    context->shading = GOURAUD_SHADING;
                    break;
                case GOURAUD_SHADING:
                    context->shading = PHONG_SHADING;
                    break;
                case PHONG_SHADING:
                    context->shading = FLAT_SHADING;
                    break;
            }
            break;
        case SDLK_2:
            context->shininess = (context->shininess == 2) ? context->shininess : context->shininess / 2;
            break;
        case SDLK_3:
            context->shininess = (context->shininess == 2 << 10) ? context->shininess : context->shininess *2;
            break;
        case SDLK_4:
            context->sphereSubdivisions = (context->sphereSubdivisions == 5) ? context->sphereSubdivisions : context->sphereSubdivisions - 5;
            break;
        case SDLK_5:
            context->sphereSubdivisions = (context->sphereSubdivisions == 150) ? context->sphereSubdivisions : context->sphereSubdivisions + 5;
            break;
        case SDLK_7:
            context->texCoordWrap = (context->texCoordWrap == TEX_COORD_CLAMP) ? TEX_COORD_REPEAT : TEX_COORD_CLAMP;
            break;
        case SDLK_8:
            context->backFaceCulling = !context->backFaceCulling;
            break;
        case SDLK_s:
            context->solarSystem = !context->solarSystem;
            break;
        default:
            break;
    }
}

// Source: http://stackoverflow.com/questions/87304/calculating-frames-per-second-in-a-game
#define MAXSAMPLES 100
int tickindex = 0;
float ticksum = 0;
float ticklist[MAXSAMPLES] = {};

float CalcAverageTick(float newtick)
{
    ticksum -= ticklist[tickindex];  
    ticksum += newtick;              
    ticklist[tickindex] = newtick;
    if (++tickindex == MAXSAMPLES)
        tickindex = 0;

    return(ticksum / MAXSAMPLES);
}

int main(int argc, char *argv[])
{
    if (!Init())
        return -1;

    //SDL_SetRelativeMouseMode(SDL_TRUE);	// Capture mouse in window

    RenderContext context = {};
    Renderer::Init(&context, SCREEN_WIDTH, SCREEN_HEIGHT);

    Uint64 performanceFrequency = SDL_GetPerformanceFrequency();

    Uint64 currentTime = SDL_GetPerformanceCounter();
    Uint64 previousTime = 0;

    SDL_Event event;

    double updateFPSTimer = 0.0f;

    while (gIsRunning)
    {
        while (SDL_PollEvent(&event) != 0)
        {
            switch (event.type)
            {
                case SDL_KEYDOWN:
                    onKeyDown(event.key.keysym.sym, &context);
                    break;
                case SDL_MOUSEMOTION:
                    onMouseMove(event, &context);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    onMouseDown(event.button.button, event.button.x, event.button.y);
                    break;
                case SDL_MOUSEBUTTONUP:
                    onMouseUp(event.button.button, event.button.x, event.button.y);
                    break;
                case SDL_MOUSEWHEEL:
                    onMouseWheel(event.wheel.y, &context);
                    break;
                case SDL_QUIT:
                    gIsRunning = false;
                    break;
                case SDL_WINDOWEVENT:
                    switch (event.window.event)
                    {
                        case SDL_WINDOWEVENT_RESIZED:
                            onWindowResized(event.window.data1, event.window.data2, &context);
                            break;
                        default:
                            break;
                    }
                default:
                    break;
            }
        }

        // FPS
        previousTime = currentTime;
        currentTime = SDL_GetPerformanceCounter();
        double dt = (currentTime - previousTime) / double(performanceFrequency);
        float avgFPS  = 1.0f / CalcAverageTick(float(dt));
        if (updateFPSTimer > 0.03)
        {
            gAverageFPS = avgFPS;
            updateFPSTimer = 0.0;
        }
        updateFPSTimer += dt;

        Renderer::Update(&context, dt, gIsRunning);

        context.mouseRelX = 0;
        context.mouseRelY = 0;
        context.mouseWheel = 0;

        RenderScreen(&context, dt);
        SDL_UpdateWindowSurface(gWindow);
    }

    Renderer::Release(&context);
    SDL_DestroyWindow(gWindow);
    gWindow = NULL;
    SDL_Quit();	// Quit SDL subsystems

    return 0;
}
