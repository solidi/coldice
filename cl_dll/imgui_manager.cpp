#include "stdio.h"
#include "stdlib.h"
#include <string>
#include <vector>
#include <string.h>

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"
#include "triangleapi.h"
#include "r_studioint.h"
#include "com_model.h"

// imgui
#include "PlatformHeaders.h"
#include <Psapi.h>
#include "SDL2/SDL.h"
#include <gl/GL.h>

#include "imgui.h"
#include "backends/imgui_impl_opengl2.h"
#include "backends/imgui_impl_sdl.h"

SDL_Window* mainWindow;
SDL_GLContext mainContext;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define GL_CLAMP_TO_EDGE 0x812F

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width = nullptr, int* out_height = nullptr)
{
	// Load from file
	int image_width = 0;
	int image_height = 0;
	unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
	if (image_data == NULL)
		return false;

	// Create a OpenGL texture identifier
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

	// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	stbi_image_free(image_data);

	*out_texture = image_texture;

	if (out_width)
		*out_width = image_width;
	if (out_height)
		*out_height = image_height;

	return true;
}

void PRECACHE_IMAGE(std::string name, GLuint* texture, int* x, int* y)
{
	std::string path = gEngfuncs.pfnGetGameDirectory() + std::string("/resource/") + name;
	bool pathCheck = LoadTextureFromFile(path.c_str(), texture, x, y);
	//	IM_ASSERT(pathCheck);
}

std::string LoadChapterName(const char* name)
{
	char *pfile, *pfile2;
	pfile = pfile2 = (char*)gEngfuncs.COM_LoadFile("resource/chapter.cfg", 5, NULL);
	char token[500];
	std::string result;

	if (pfile == nullptr)
	{
		return result;
	}

	while (pfile = gEngfuncs.COM_ParseFile(pfile, token))
	{
		if (!stricmp(token, name))
		{
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			if (token)
			{
				pfile = gEngfuncs.COM_ParseFile(pfile, token);
				result = std::string(token);
				break;
			}
		}
	}

	gEngfuncs.COM_FreeFile(pfile2);
	pfile = pfile2 = nullptr;

	return result;
}

std::string LoadChapterConfig(const char* name)
{
	char *pfile, *pfile2;
	pfile = pfile2 = (char*)gEngfuncs.COM_LoadFile("resource/chapter.cfg", 5, NULL);
	char token[500];
	std::string result;

	if (pfile == nullptr)
	{
		return result;
	}

	while (pfile = gEngfuncs.COM_ParseFile(pfile, token))
	{
		if (!stricmp(token, name))
		{
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			result = std::string(token);
			break;
		}
	}

	gEngfuncs.COM_FreeFile(pfile2);
	pfile = pfile2 = nullptr;

	return result;
}

void ClientImGui_HookedDraw()
{
	g_ImGUIManager.Draw();
	SDL_GL_SwapWindow(mainWindow);
}

int ClientImGui_EventWatch(void* data, SDL_Event* event)
{
	return ImGui_ImplSDL2_ProcessEvent(event);
}

void ClientImGui_HWHook()
{
	// Thanks to half payne's developer for the idea
	// Changed some types for constant size things

#pragma warning(disable : 6387)

	unsigned int origin = 0;

	MODULEINFO moduleInfo;
	if (GetModuleInformation(GetCurrentProcess(), GetModuleHandle("hw.dll"), &moduleInfo, sizeof(moduleInfo)))
	{
		origin = (unsigned int)moduleInfo.lpBaseOfDll;

		int8_t* slice = new int8_t[1048576];
		ReadProcessMemory(GetCurrentProcess(), (const void*)origin, slice, 1048576, nullptr);

		// Predefined magic stuff
		uint8_t magic[] = {0x8B, 0x4D, 0x08, 0x83, 0xC4, 0x08, 0x89, 0x01, 0x5D, 0xC3, 0x90, 0x90, 0x90, 0x90, 0x90, 0xA1};

		for (unsigned int i = 0; i < 1048576 - 16; i++)
		{
			bool sequenceIsMatching = memcmp(slice + i, magic, 16) == 0;
			if (sequenceIsMatching)
			{
				origin += i + 27;
				break;
			}
		}

		delete[] slice;

		int8_t opCode[1];
		ReadProcessMemory(GetCurrentProcess(), (const void*)origin, opCode, 1, nullptr);
		if (opCode[0] != 0xFFFFFFE8)
		{
			gEngfuncs.Con_DPrintf("Failed to embed ImGUI. couldn't find opCode.\n");
			return;
		}
	}
	else
	{
		gEngfuncs.Con_DPrintf("Failed to embed ImGUI: failed to get hw.dll memory base address.\n");
		return;
	}

	ImGui_ImplOpenGL2_Init();
	ImGui_ImplSDL2_InitForOpenGL(mainWindow, ImGui::GetCurrentContext());

	// To make a detour, an offset to dedicated function must be calculated and then correctly replaced
	unsigned int detourFunctionAddress = (unsigned int)&ClientImGui_HookedDraw;
	unsigned int offset = (detourFunctionAddress)-origin - 5;

	// Little endian offset
	uint8_t offsetBytes[4];
	for (int i = 0; i < 4; i++)
	{
		offsetBytes[i] = (offset >> (i * 8));
	}

	// This is WinAPI call, blatantly overwriting the memory with raw pointer would crash the program
	// Notice the 1 byte offset from the origin
	WriteProcessMemory(GetCurrentProcess(), (void*)(origin + 1), offsetBytes, 4, nullptr);

	SDL_AddEventWatch(ClientImGui_EventWatch, nullptr);

#pragma warning(default : 6387)
}

// image loading
int binX, binY, NewGameSizeX, NewGameSizeY, ExitSizeX, ExitSizeY, startSizeX, startSizeY, cancelSizeX, cancelSizeY, nextSizeX, nextSizeY,
	backSizeX, backSizeY;
GLuint newgame = 0;
GLuint exitbtn = 0;
GLuint startbtn;
GLuint cancelbtn;
GLuint nextbtn, backbtn;

GLuint thumbnail0 = 0, thumbnail1 = 0, thumbnail2 = 0;
GLuint thumbnail3 = 0, thumbnail4 = 0, thumbnail5 = 0;
GLuint thumbnail6 = 0, thumbnail7 = 0, thumbnail8 = 0;
GLuint thumbnail9 = 0, thumbnail10 = 0, thumbnail11 = 0;
GLuint thumbnail12 = 0, thumbnail13 = 0, thumbnail14 = 0;
GLuint thumbnail15 = 0, thumbnail16 = 0, thumbnail17 = 0;
GLuint thumbnail18 = 0, thumbnail19 = 0;

void __CmdFunc_OpenChapter()
{
	if (gEngfuncs.GetMaxClients() > 1) {
		gEngfuncs.Con_Printf("Must be in single player mode!\n");
		return;
	}

	EngineClientCmd("disconnect");
	g_ImGUIManager.isMenuOpen = !g_ImGUIManager.isMenuOpen;
}

bool CImguiManager::Init()
{
	const char *modName = gEngfuncs.pfnGetGameDirectory();

	if (modName == NULL)
		return false;

	// Must be _sp folder
	if (!strstr(modName, "_sp"))
		return false;

	HOOK_COMMAND("imgui_chapter", OpenChapter);
	mainWindow = SDL_GetWindowFromID(1);
	// mainContext = SDL_GL_CreateContext(mainWindow);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
	std::string path = gEngfuncs.pfnGetGameDirectory() + std::string("/resource/fonts/") + LoadChapterConfig("fontname");
	io.Fonts->AddFontFromFileTTF(path.c_str(), atof(LoadChapterConfig("fontsize").c_str()));

	// For Overdraw
	ClientImGui_HWHook();

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// load some textures
	PRECACHE_IMAGE(LoadChapterConfig("logo"), &newgame, &NewGameSizeX, &NewGameSizeY);
	PRECACHE_IMAGE(LoadChapterConfig("exitbtn"), &exitbtn, &ExitSizeX, &ExitSizeY);
	PRECACHE_IMAGE(LoadChapterConfig("training"), &thumbnail0, &binX, &binY);
	PRECACHE_IMAGE(LoadChapterConfig("chapter1"), &thumbnail1, &binX, &binY);
	PRECACHE_IMAGE(LoadChapterConfig("chapter2"), &thumbnail2, &binX, &binY);
	PRECACHE_IMAGE(LoadChapterConfig("chapter3"), &thumbnail3, &binX, &binY);
	PRECACHE_IMAGE(LoadChapterConfig("chapter4"), &thumbnail4, &binX, &binY);
	PRECACHE_IMAGE(LoadChapterConfig("chapter5"), &thumbnail5, &binX, &binY);
	PRECACHE_IMAGE(LoadChapterConfig("chapter6"), &thumbnail6, &binX, &binY);
	PRECACHE_IMAGE(LoadChapterConfig("chapter7"), &thumbnail7, &binX, &binY);
	PRECACHE_IMAGE(LoadChapterConfig("chapter8"), &thumbnail8, &binX, &binY);
	PRECACHE_IMAGE(LoadChapterConfig("chapter9"), &thumbnail9, &binX, &binY);
	PRECACHE_IMAGE(LoadChapterConfig("chapter10"), &thumbnail10, &binX, &binY);
	PRECACHE_IMAGE(LoadChapterConfig("chapter11"), &thumbnail11, &binX, &binY);
	PRECACHE_IMAGE(LoadChapterConfig("chapter12"), &thumbnail12, &binX, &binY);
	PRECACHE_IMAGE(LoadChapterConfig("chapter13"), &thumbnail13, &binX, &binY);
	PRECACHE_IMAGE(LoadChapterConfig("chapter14"), &thumbnail14, &binX, &binY);
	PRECACHE_IMAGE(LoadChapterConfig("chapter15"), &thumbnail15, &binX, &binY);
	PRECACHE_IMAGE(LoadChapterConfig("chapter16"), &thumbnail16, &binX, &binY);
	PRECACHE_IMAGE(LoadChapterConfig("chapter17"), &thumbnail17, &binX, &binY);
	PRECACHE_IMAGE(LoadChapterConfig("chapter18"), &thumbnail18, &binX, &binY);
	PRECACHE_IMAGE(LoadChapterConfig("chapter19"), &thumbnail19, &binX, &binY);
	PRECACHE_IMAGE(LoadChapterConfig("startbtn"), &startbtn, &startSizeX, &startSizeY);
	PRECACHE_IMAGE(LoadChapterConfig("cancelbtn"), &cancelbtn, &cancelSizeX, &cancelSizeY);
	PRECACHE_IMAGE(LoadChapterConfig("nextbtn"), &nextbtn, &nextSizeX, &nextSizeY);
	PRECACHE_IMAGE(LoadChapterConfig("backbtn"), &backbtn, &backSizeX, &backSizeY);

	return true;
}

bool CImguiManager::VidInit()
{
	return true;
}

// selected button
char selectedChapter[512];
bool selectedPanel[3];

void CImguiManager::Draw()
{
	// read old skill value
	if (!skillMode[0] && !skillMode[1] && !skillMode[2] && !skillMode[3])
	{
		int skillValue = static_cast<int>(CVAR_GET_FLOAT("skill") - 1);
		skillMode[skillValue] = true;
	}

	// draw
	if (strlen(gEngfuncs.pfnGetLevelName()) < 4)
	{
		// ================== call drawing code between this ==================

		if (isMenuOpen)
		{
			ImGui_ImplOpenGL2_NewFrame();
			ImGui_ImplSDL2_NewFrame(mainWindow);
			ImGui::NewFrame();

			DrawChapter();

			// glViewport( 0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y );
			ImGui::Render();
			ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
		}

		// ================== call drawing code between this ==================
	}
	else
	{
		selectedPanel[0] = false;
		selectedPanel[1] = false;
		selectedPanel[2] = false;
		strcpy(selectedChapter, "");
		isMenuOpen = false;
	}

	// update discord here
	//g_DiscordRPC.Update();
}

void DrawFirstPage(ImVec4* colours)
{
	// column1
	ImGui::Columns(3);
	ImGui::SetColumnOffset(0, 0);
	colours[ImGuiCol_Text] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	ImGui::Text("TRAINING");
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	ImGui::Text(LoadChapterName("training").c_str());

	if (selectedPanel[0])
	{
		colours[ImGuiCol_Button] = ImVec4(0.0f, 0.44f, 0.9f, 1);
		colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	}

	if (ImGui::ImageButton((void*)(intptr_t)thumbnail0, ImVec2(200, 110)))
	{
		// EngineClientCmd("map hle_t0a0.bsp");
		strcpy(selectedChapter, "map t0a0.bsp");
		selectedPanel[0] = true;
		selectedPanel[1] = false;
		selectedPanel[2] = false;
	}
	colours[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1);

	// column2
	ImGui::NextColumn();
	ImGui::SetColumnOffset(1, 230);
	colours[ImGuiCol_Text] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	ImGui::Text("CHAPTER 1");
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	ImGui::Text(LoadChapterName("chapter1").c_str());

	if (selectedPanel[1])
	{
		colours[ImGuiCol_Button] = ImVec4(0.0f, 0.44f, 0.9f, 1);
		colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	}

	if (ImGui::ImageButton((void*)(intptr_t)thumbnail1, ImVec2(200, 110)))
	{
		// EngineClientCmd("map c0a0.bsp");
		strcpy(selectedChapter, "map c0a0.bsp");
		selectedPanel[0] = false;
		selectedPanel[1] = true;
		selectedPanel[2] = false;
	}
	colours[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1);


	// column3
	ImGui::NextColumn();
	ImGui::SetColumnOffset(2, 460);
	colours[ImGuiCol_Text] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	ImGui::Text("CHAPTER 2");
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	ImGui::Text(LoadChapterName("chapter2").c_str());

	if (selectedPanel[2])
	{
		colours[ImGuiCol_Button] = ImVec4(0.0f, 0.44f, 0.9f, 1);
		colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	}

	if (ImGui::ImageButton((void*)(intptr_t)thumbnail2, ImVec2(200, 110)))
	{
		// EngineClientCmd("map hle_c1a0.bsp");
		strcpy(selectedChapter, "map c1a0.bsp");
		selectedPanel[0] = false;
		selectedPanel[1] = false;
		selectedPanel[2] = true;
	}
	colours[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1);
}

void DrawSecondPage(ImVec4* colours)
{
	// column1
	ImGui::Columns(3);
	ImGui::SetColumnOffset(0, 0);
	colours[ImGuiCol_Text] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	ImGui::Text("CHAPTER 3");
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	ImGui::Text(LoadChapterName("chapter3").c_str());

	if (selectedPanel[0])
	{
		colours[ImGuiCol_Button] = ImVec4(0.0f, 0.44f, 0.9f, 1);
		colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	}

	if (ImGui::ImageButton((void*)(intptr_t)thumbnail3, ImVec2(200, 110)))
	{
		// EngineClientCmd("map hle_c1a1b.bsp");
		strcpy(selectedChapter, "map c1a0c.bsp");
		selectedPanel[0] = true;
		selectedPanel[1] = false;
		selectedPanel[2] = false;
	}
	colours[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1);


	// column2
	ImGui::NextColumn();
	ImGui::SetColumnOffset(1, 230);
	colours[ImGuiCol_Text] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	ImGui::Text("CHAPTER 4");
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	ImGui::Text(LoadChapterName("chapter4").c_str());

	if (selectedPanel[1])
	{
		colours[ImGuiCol_Button] = ImVec4(0.0f, 0.44f, 0.9f, 1);
		colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	}

	if (ImGui::ImageButton((void*)(intptr_t)thumbnail4, ImVec2(200, 110)))
	{
		// EngineClientCmd("map hle_c1a2.bsp");
		strcpy(selectedChapter, "map c1a2.bsp");
		selectedPanel[0] = false;
		selectedPanel[1] = true;
		selectedPanel[2] = false;
	}
	colours[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1);


	// column3
	ImGui::NextColumn();
	ImGui::SetColumnOffset(2, 460);
	colours[ImGuiCol_Text] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	ImGui::Text("CHAPTER 5");
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	ImGui::Text(LoadChapterName("chapter5").c_str());

	if (selectedPanel[2])
	{
		colours[ImGuiCol_Button] = ImVec4(0.0f, 0.44f, 0.9f, 1);
		colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	}

	if (ImGui::ImageButton((void*)(intptr_t)thumbnail5, ImVec2(200, 110)))
	{
		// EngineClientCmd("map hle_c1a3.bsp");
		strcpy(selectedChapter, "map c1a3.bsp");
		selectedPanel[0] = false;
		selectedPanel[1] = false;
		selectedPanel[2] = true;
	}
	colours[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1);
}

void DrawThirdPage(ImVec4* colours)
{
	// column1
	ImGui::Columns(3);
	ImGui::SetColumnOffset(0, 0);
	colours[ImGuiCol_Text] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	ImGui::Text("CHAPTER 6");
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	ImGui::Text(LoadChapterName("chapter6").c_str());

	if (selectedPanel[0])
	{
		colours[ImGuiCol_Button] = ImVec4(0.0f, 0.44f, 0.9f, 1);
		colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	}

	if (ImGui::ImageButton((void*)(intptr_t)thumbnail6, ImVec2(200, 110)))
	{
		strcpy(selectedChapter, "map c1a4.bsp");
		selectedPanel[0] = true;
		selectedPanel[1] = false;
		selectedPanel[2] = false;
	}
	colours[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1);


	// column2
	ImGui::NextColumn();
	ImGui::SetColumnOffset(1, 230);
	colours[ImGuiCol_Text] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	ImGui::Text("CHAPTER 7");
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	ImGui::Text(LoadChapterName("chapter7").c_str());

	if (selectedPanel[1])
	{
		colours[ImGuiCol_Button] = ImVec4(0.0f, 0.44f, 0.9f, 1);
		colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	}

	if (ImGui::ImageButton((void*)(intptr_t)thumbnail7, ImVec2(200, 110)))
	{
		strcpy(selectedChapter, "map c2a1.bsp");
		selectedPanel[0] = false;
		selectedPanel[1] = true;
		selectedPanel[2] = false;
	}
	colours[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1);


	// column3
	ImGui::NextColumn();
	ImGui::SetColumnOffset(2, 460);
	colours[ImGuiCol_Text] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	ImGui::Text("CHAPTER 8");
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	ImGui::Text(LoadChapterName("chapter8").c_str());

	if (selectedPanel[2])
	{
		colours[ImGuiCol_Button] = ImVec4(0.0f, 0.44f, 0.9f, 1);
		colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	}

	if (ImGui::ImageButton((void*)(intptr_t)thumbnail8, ImVec2(200, 110)))
	{
		strcpy(selectedChapter, "map c2a2.bsp");
		selectedPanel[0] = false;
		selectedPanel[1] = false;
		selectedPanel[2] = true;
	}
	colours[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1);
}

void DrawFourthPage(ImVec4* colours)
{
	// column1
	ImGui::Columns(3);
	ImGui::SetColumnOffset(0, 0);
	colours[ImGuiCol_Text] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	ImGui::Text("CHAPTER 9");
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	ImGui::Text(LoadChapterName("chapter9").c_str());

	if (selectedPanel[0])
	{
		colours[ImGuiCol_Button] = ImVec4(0.0f, 0.44f, 0.9f, 1);
		colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	}

	if (ImGui::ImageButton((void*)(intptr_t)thumbnail9, ImVec2(200, 110)))
	{
		strcpy(selectedChapter, "map c2a3.bsp");
		selectedPanel[0] = true;
		selectedPanel[1] = false;
		selectedPanel[2] = false;
	}
	colours[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1);


	// column2
	ImGui::NextColumn();
	ImGui::SetColumnOffset(1, 230);
	colours[ImGuiCol_Text] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	ImGui::Text("CHAPTER 10");
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	ImGui::Text(LoadChapterName("chapter10").c_str());

	if (selectedPanel[1])
	{
		colours[ImGuiCol_Button] = ImVec4(0.0f, 0.44f, 0.9f, 1);
		colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	}

	if (ImGui::ImageButton((void*)(intptr_t)thumbnail10, ImVec2(200, 110)))
	{
		strcpy(selectedChapter, "map c2a4.bsp");
		selectedPanel[0] = false;
		selectedPanel[1] = true;
		selectedPanel[2] = false;
	}
	colours[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1);


	// column3
	ImGui::NextColumn();
	ImGui::SetColumnOffset(2, 460);
	colours[ImGuiCol_Text] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	ImGui::Text("CHAPTER 11");
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	ImGui::Text(LoadChapterName("chapter11").c_str());

	if (selectedPanel[2])
	{
		colours[ImGuiCol_Button] = ImVec4(0.0f, 0.44f, 0.9f, 1);
		colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	}

	if (ImGui::ImageButton((void*)(intptr_t)thumbnail11, ImVec2(200, 110)))
	{
		strcpy(selectedChapter, "map c2a4d.bsp");
		selectedPanel[0] = false;
		selectedPanel[1] = false;
		selectedPanel[2] = true;
	}
	colours[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1);
}


void DrawFifthPage(ImVec4* colours)
{
	// column1
	ImGui::Columns(3);
	ImGui::SetColumnOffset(0, 0);
	colours[ImGuiCol_Text] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	ImGui::Text("CHAPTER 12");
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	ImGui::Text(LoadChapterName("chapter12").c_str());

	if (selectedPanel[0])
	{
		colours[ImGuiCol_Button] = ImVec4(0.0f, 0.44f, 0.9f, 1);
		colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	}

	if (ImGui::ImageButton((void*)(intptr_t)thumbnail12, ImVec2(200, 110)))
	{
		strcpy(selectedChapter, "map c2a5.bsp");
		selectedPanel[0] = true;
		selectedPanel[1] = false;
		selectedPanel[2] = false;
	}
	colours[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1);


	// column2
	ImGui::NextColumn();
	ImGui::SetColumnOffset(1, 230);
	colours[ImGuiCol_Text] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	ImGui::Text("CHAPTER 13");
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	ImGui::Text(LoadChapterName("chapter13").c_str());

	if (selectedPanel[1])
	{
		colours[ImGuiCol_Button] = ImVec4(0.0f, 0.44f, 0.9f, 1);
		colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	}

	if (ImGui::ImageButton((void*)(intptr_t)thumbnail13, ImVec2(200, 110)))
	{
		strcpy(selectedChapter, "map c3a1.bsp");
		selectedPanel[0] = false;
		selectedPanel[1] = true;
		selectedPanel[2] = false;
	}
	colours[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1);


	// column3
	ImGui::NextColumn();
	ImGui::SetColumnOffset(2, 460);
	colours[ImGuiCol_Text] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	ImGui::Text("CHAPTER 14");
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	ImGui::Text(LoadChapterName("chapter14").c_str());

	if (selectedPanel[2])
	{
		colours[ImGuiCol_Button] = ImVec4(0.0f, 0.44f, 0.9f, 1);
		colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	}

	if (ImGui::ImageButton((void*)(intptr_t)thumbnail14, ImVec2(200, 110)))
	{
		strcpy(selectedChapter, "map c3a2e.bsp");
		selectedPanel[0] = false;
		selectedPanel[1] = false;
		selectedPanel[2] = true;
	}
	colours[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1);
}

void DrawSixthPage(ImVec4* colours)
{
	// column1
	ImGui::Columns(3);
	ImGui::SetColumnOffset(0, 0);
	colours[ImGuiCol_Text] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	ImGui::Text("CHAPTER 15");
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	ImGui::Text(LoadChapterName("chapter15").c_str());

	if (selectedPanel[0])
	{
		colours[ImGuiCol_Button] = ImVec4(0.0f, 0.44f, 0.9f, 1);
		colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	}

	if (ImGui::ImageButton((void*)(intptr_t)thumbnail15, ImVec2(200, 110)))
	{
		strcpy(selectedChapter, "map c4a1.bsp");
		selectedPanel[0] = true;
		selectedPanel[1] = false;
		selectedPanel[2] = false;
	}
	colours[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1);


	// column2
	ImGui::NextColumn();
	ImGui::SetColumnOffset(1, 230);
	colours[ImGuiCol_Text] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	ImGui::Text("CHAPTER 16");
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	ImGui::Text(LoadChapterName("chapter16").c_str());

	if (selectedPanel[1])
	{
		colours[ImGuiCol_Button] = ImVec4(0.0f, 0.44f, 0.9f, 1);
		colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	}

	if (ImGui::ImageButton((void*)(intptr_t)thumbnail16, ImVec2(200, 110)))
	{
		strcpy(selectedChapter, "map c4a2.bsp");
		selectedPanel[0] = false;
		selectedPanel[1] = true;
		selectedPanel[2] = false;
	}
	colours[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1);


	// column3
	ImGui::NextColumn();
	ImGui::SetColumnOffset(2, 460);
	colours[ImGuiCol_Text] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	ImGui::Text("CHAPTER 17");
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	ImGui::Text(LoadChapterName("chapter17").c_str());

	if (selectedPanel[2])
	{
		colours[ImGuiCol_Button] = ImVec4(0.0f, 0.44f, 0.9f, 1);
		colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	}

	if (ImGui::ImageButton((void*)(intptr_t)thumbnail17, ImVec2(200, 110)))
	{
		strcpy(selectedChapter, "map c4a1a.bsp");
		selectedPanel[0] = false;
		selectedPanel[1] = false;
		selectedPanel[2] = true;
	}
	colours[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1);
}

void DrawSeventhPage(ImVec4* colours)
{
	// column1
	ImGui::Columns(3);
	ImGui::SetColumnOffset(0, 0);
	colours[ImGuiCol_Text] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	ImGui::Text("CHAPTER 18");
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	ImGui::Text(LoadChapterName("chapter18").c_str());

	if (selectedPanel[0])
	{
		colours[ImGuiCol_Button] = ImVec4(0.0f, 0.44f, 0.9f, 1);
		colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	}

	if (ImGui::ImageButton((void*)(intptr_t)thumbnail18, ImVec2(200, 110)))
	{
		strcpy(selectedChapter, "map c4a3.bsp");
		selectedPanel[0] = true;
		selectedPanel[1] = false;
		selectedPanel[2] = false;
	}
	colours[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1);


	// column2
	ImGui::NextColumn();
	ImGui::SetColumnOffset(1, 230);
	colours[ImGuiCol_Text] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	ImGui::Text("CHAPTER 19");
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	ImGui::Text(LoadChapterName("chapter19").c_str());

	if (selectedPanel[1])
	{
		colours[ImGuiCol_Button] = ImVec4(0.0f, 0.44f, 0.9f, 1);
		colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	}

	if (ImGui::ImageButton((void*)(intptr_t)thumbnail19, ImVec2(200, 110)))
	{
		strcpy(selectedChapter, "map c5a1.bsp");
		selectedPanel[0] = false;
		selectedPanel[1] = true;
		selectedPanel[2] = false;
	}
	colours[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1);


	// column3
	ImGui::NextColumn();
	ImGui::SetColumnOffset(2, 460);
/*
	colours[ImGuiCol_Text] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	ImGui::Text("CHAPTER 20");
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	ImGui::Text(LoadChapterName("chapter20").c_str());

	if (selectedPanel[2])
	{
		colours[ImGuiCol_Button] = ImVec4(0.0f, 0.44f, 0.9f, 1);
		colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	}

	if (ImGui::ImageButton((void*)(intptr_t)thumbnail20, ImVec2(200, 110)))
	{
		strcpy(selectedChapter, "map c4a1a.bsp");
		selectedPanel[0] = false;
		selectedPanel[1] = false;
		selectedPanel[2] = true;
	}
	colours[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 1);
*/
}

void CImguiManager::DrawChapter()
{
	// setup
	bool is_open;
	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_NoResize;
	window_flags |= ImGuiWindowFlags_NoTitleBar;
	window_flags |= ImGuiWindowFlags_NoCollapse;
	window_flags |= ImGuiWindowFlags_NoScrollbar;
	window_flags |= ImGuiWindowFlags_NoScrollWithMouse;

	// get resolution
	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	// ImGui::SetNextWindowPos(ImVec2(22, io.DisplaySize.y - 218));
	ImGui::SetNextWindowSize(ImVec2(685, 305));

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// override imgui styles
	ImVec4* colours = ImGui::GetStyle().Colors;
	colours[ImGuiCol_TitleBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
	colours[ImGuiCol_TitleBgActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
	colours[ImGuiCol_TitleBgCollapsed] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
	colours[ImGuiCol_WindowBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
	colours[ImGuiCol_Text] = ImVec4(0.78f, 0.78f, 0.78f, 1);
	colours[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
	colours[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.44f, 0.9f, 0);
	colours[ImGuiCol_FrameBg] = ImVec4(0.19f, 0.19f, 0.19f, 1);
	colours[ImGuiCol_FrameBgActive] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	colours[ImGuiCol_CheckMark] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	colours[ImGuiCol_FrameBgHovered] = ImVec4(0.0f, 0.44f, 0.9f, 1);

	ImGui::Begin("NEW GAME", &is_open, window_flags);

	// window title
	ImGui::Columns(2, 0, false);
	// ImGui::SetColumnOffset(1, 645);
	ImGui::Image((void*)(intptr_t)newgame, ImVec2(NewGameSizeX, NewGameSizeY));
	ImGui::NextColumn();
	ImGui::SetCursorPosX(650);
	if (ImGui::ImageButton((void*)(intptr_t)exitbtn, ImVec2(ExitSizeX, ExitSizeY)))
	{
		selectedPanel[0] = false;
		selectedPanel[1] = false;
		selectedPanel[2] = false;
		strcpy(selectedChapter, "");
		isMenuOpen = false;
	}
	ImGui::Columns(1);

	// pages
	colours[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.44f, 0.9f, 1);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.0, 0.0f, 0.0f, 1.0f);
	colours[ImGuiCol_Button] = ImVec4(0.0, 0.0f, 0.0f, 1.0f);
	switch (page)
	{
	case 0: DrawFirstPage(colours); break;
	case 1: DrawSecondPage(colours); break;
	case 2: DrawThirdPage(colours); break;
	case 3: DrawFourthPage(colours); break;
	case 4: DrawFifthPage(colours); break;
	case 5: DrawSixthPage(colours); break;
	case 6: DrawSeventhPage(colours); break;
	}
	colours[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
	colours[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);

	// next button
	colours[ImGuiCol_ButtonActive] = ImVec4(0.19f, 0.19f, 0.19f, 0);
	ImGui::Columns(2, 0, false);

	if (page != 0)
	{
		float y = ImGui::GetCursorPosY();
		ImGui::SetCursorPosX(8);
		if (ImGui::ImageButton((void*)(intptr_t)backbtn, ImVec2(backSizeX, backSizeY)))
		{
			selectedPanel[0] = false;
			selectedPanel[1] = false;
			selectedPanel[2] = false;
			strcpy(selectedChapter, "");
			page--;
		}

		ImGui::SetCursorPosY(y + 6);
		ImGui::SetCursorPosX(18);
		ImGui::Text(LoadChapterName("backbtn").c_str());
	}
	else
		ImGui::Text("");

	ImGui::NextColumn();

	if (page < 6)
	{
		float y = ImGui::GetCursorPosY();
		ImGui::SetCursorPosX(567);
		if (ImGui::ImageButton((void*)(intptr_t)nextbtn, ImVec2(nextSizeX, nextSizeY)))
		{
			selectedPanel[0] = false;
			selectedPanel[1] = false;
			selectedPanel[2] = false;
			strcpy(selectedChapter, "");
			page++;
		}

		ImGui::SetCursorPosY(y + 6);
		ImGui::SetCursorPosX(577);
		ImGui::Text(LoadChapterName("nextbtn").c_str());
	}
	else
		ImGui::Text("");

	// space
	ImGui::Columns(1);
	ImGui::Text("");
	ImGui::Separator();

	// difficulty text
	ImGui::Columns(1);
	ImGui::Text("Difficulty :");

	// skill buttons and start new game
	ImGui::Columns(6, 0, false);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

	// first
	// ImGui::Spacing();
	ImGui::Spacing();
	ImGui::SetColumnOffset(0, 0);

	if (ImGui::Checkbox("Easy", &skillMode[0]))
	{
		EngineClientCmd("skill 1");
		skillMode[1] = false;
		skillMode[2] = false;
		skillMode[3] = false;
	}

	// second
	ImGui::NextColumn();
	// ImGui::Spacing();
	ImGui::Spacing();
	ImGui::SetColumnOffset(1, 100);
	if (ImGui::Checkbox("Medium", &skillMode[1]))
	{
		EngineClientCmd("skill 2");
		skillMode[0] = false;
		skillMode[2] = false;
		skillMode[3] = false;
	}

	// third
	ImGui::NextColumn();
	// ImGui::Spacing();
	ImGui::Spacing();
	ImGui::SetColumnOffset(2, 200);
	if (ImGui::Checkbox("Hard", &skillMode[2]))
	{
		EngineClientCmd("skill 3");
		skillMode[0] = false;
		skillMode[1] = false;
		skillMode[3] = false;
	}

	// forth
	ImGui::NextColumn();
	// ImGui::Spacing();
	ImGui::Spacing();
	ImGui::SetColumnOffset(3, 300);
	/*if (ImGui::Checkbox("Extended", &skillMode[3]))
	{
		EngineClientCmd("skill 4");
		skillMode[0] = false;
		skillMode[1] = false;
		skillMode[2] = false;
	}*/
	ImGui::PopStyleVar();

	// start new game button
	ImGui::NextColumn();
	float yStart = ImGui::GetCursorPosY();
	ImGui::SetColumnOffset(4, 455);
	if (ImGui::ImageButton((void*)(intptr_t)startbtn, ImVec2(startSizeX, startSizeY)))
	{
		EngineClientCmd(selectedChapter);
		selectedPanel[0] = false;
		selectedPanel[1] = false;
		selectedPanel[2] = false;
		strcpy(selectedChapter, "");
		isMenuOpen = false;
	}
	ImGui::SetCursorPosY(yStart + 6);
	ImGui::SetCursorPosX(473);
	ImGui::Text(LoadChapterName("startbtn").c_str());

	// cancel button
	ImGui::NextColumn();
	ImGui::SetColumnOffset(5, 595);
	if (ImGui::ImageButton((void*)(intptr_t)cancelbtn, ImVec2(cancelSizeX, cancelSizeY)))
	{
		selectedPanel[0] = false;
		selectedPanel[1] = false;
		selectedPanel[2] = false;
		strcpy(selectedChapter, "");
		isMenuOpen = false;
	}
	ImGui::SetCursorPosY(yStart + 6);
	ImGui::SetCursorPosX(613);
	ImGui::Text(LoadChapterName("cancelbtn").c_str());

	ImGui::End();
}
