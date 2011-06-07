/*********************************************************\
 *  File: Application.cpp                                *
 *
 *  Copyright (C) 2002-2011 The PixelLight Team (http://www.pixellight.org/)
 *
 *  This file is part of PixelLight.
 *
 *  PixelLight is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  PixelLight is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with PixelLight. If not, see <http://www.gnu.org/licenses/>.
\*********************************************************/


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <PLGeneral/File/Url.h>
#include <PLGeneral/System/System.h>
#include <PLCore/Base/Class.h>
#include <PLCore/Tools/Localization.h>
#include <PLCore/Tools/LoadableManager.h>
#include <PLScript/Script.h>
#include <PLScript/FuncScriptPtr.h>
#include <PLScript/ScriptManager.h>
#include <PLGui/Gui/Gui.h>
#include <PLGui/Gui/Base/Keys.h>
#include <PLGui/Widgets/Widget.h>
#include <PLRenderer/RendererContext.h>
#include <PLRenderer/Material/MaterialManager.h>
#include <PLRenderer/Material/ParameterManager.h>
#include <PLScene/Compositing/SceneRenderer.h>
#include <PLScene/Scene/SPScene.h>
#include <PLScene/Scene/SceneContext.h>
#include <PLScene/Scene/SceneContainer.h>
#include <PLScene/Scene/SceneNodeModifier.h>
#include <PLScene/Scene/SceneNodes/Console/SNConsoleBase.h>
#include <PLEngine/Gui/RenderWindow.h>
#include "Interaction.h"
#include "Application.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
using namespace PLGeneral;
using namespace PLCore;
using namespace PLScript;
using namespace PLGui;
using namespace PLRenderer;
using namespace PLScene;
using namespace PLEngine;


//[-------------------------------------------------------]
//[ RTTI interface                                        ]
//[-------------------------------------------------------]
pl_implement_class(Application)


//[-------------------------------------------------------]
//[ Public static data                                    ]
//[-------------------------------------------------------]
// A helper to be able to toggle between 'choose scene file at start' and 'load
// default scene at start' (quite comfortable if you make many experiments :)
const String Application::DefaultScene = "Data/Scenes/Dungeon.scene";


//[-------------------------------------------------------]
//[ Public functions                                      ]
//[-------------------------------------------------------]
/**
*  @brief
*    Constructor
*/
Application::Application() : BasicSceneApplication(),
	EventHandlerLoadProgress(&Application::OnLoadProgress, this),
	m_pScript(nullptr),
	m_pInteraction(nullptr),
	m_fLoadProgress(0.0f)
{
	// Set application name and title
	SetName("Dungeon");
	SetTitle(PLT("PixelLight dungeon demo"));
	SetAppDataSubdir(System::GetInstance()->GetDataDirName("PixelLight"));

	// The demo is published as a simple archive, so, put the log and configuration files in the same directory the executable is
	// in - as a result, the user only has to remove this directory and the demo is completly gone from the system :D
	SetMultiUser(false);

	// This application accepts all the standard parameters that are defined in the application
	// base class (such as --help etc.). The last parameter however is the filename to load, so add that.
	m_cCommandLine.AddArgument("Filename", "Scene filename", "", false);
	m_cCommandLine.AddFlag("Expert", "-e", "--expert", "Expert mode, no additional help texts", false);
	m_cCommandLine.AddFlag("Repeat", "-r", "--repeat", "If movie and making of is finished, start the movie again instead of switching to ínteractive mode", false);
}

/**
*  @brief
*    Destructor
*/
Application::~Application()
{
	// Destroy the interaction component
	if (m_pInteraction)
		delete m_pInteraction;
}

/**
*  @brief
*    Returns whether or not the application runs within the expert mode
*/
bool Application::IsExpertMode() const
{
	// Check 'Expert' commando line flag
	return m_cCommandLine.IsValueSet("Expert");
}

/**
*  @brief
*    Returns whether or not the application runs within the repeat mode
*/
bool Application::IsRepeatMode() const
{
	// Check 'Repeat' commando line flag
	return m_cCommandLine.IsValueSet("Repeat");
}

/**
*  @brief
*    Returns the interaction component
*/
Interaction *Application::GetInteraction()
{
	return m_pInteraction;
}

/**
*  @brief
*    Loads a scene
*/
bool Application::LoadScene(const String &sFilename)
{
	// Destroy the previous interaction component
	if (m_pInteraction) {
		delete m_pInteraction;
		m_pInteraction = nullptr;
	}

	// Reset the current load progress
	m_fLoadProgress = 0.0f;

	// Remove the base directory of the previously loaded scene from the loadable manager
	if (m_sCurrentSceneBaseDirectory.GetLength())
		LoadableManager::GetInstance()->RemoveBaseDir(m_sCurrentSceneBaseDirectory);
	m_sCurrentSceneBaseDirectory = "";

	{ // Add the base directory of the scene to load is in
		// Validate path
		const String sDirectory = Url(sFilename).Collapse().CutFilename();

		// Search for "/Data/Scenes/" and get the prefix of that
		int nIndex = sDirectory.IndexOf("/Data/Scenes/");
		if (nIndex >= 0)
			m_sCurrentSceneBaseDirectory = sDirectory.GetSubstring(0, nIndex);
		m_sCurrentSceneBaseDirectory = "file://" + m_sCurrentSceneBaseDirectory + '/';

		// Add the base directory of the current scene to the loadable manager
		LoadableManager::GetInstance()->AddBaseDir(m_sCurrentSceneBaseDirectory);
	}

	// Load the scene
	const bool bResult = BasicSceneApplication::LoadScene(sFilename);

	// Get the renderer context
	RendererContext *pRendererContext = GetRendererContext();
	if (pRendererContext) {
		// Give the "DoorGlow" material an animated emissive map for a more impressive god rays effect and enhance the diffuse color for more glow
		Material *pMaterial = pRendererContext->GetMaterialManager().GetByName("Data\\Materials\\Dungeon\\DoorGlow.mat");
		if (pMaterial) {
			// Set an animated emissive map
			pMaterial->GetParameterManager().SetParameterString("EmissiveMap", "Data/Textures/Caust.tani");

			// Enhance the diffuse color for more glow
			pMaterial->GetParameterManager().SetParameter3f("DiffuseColor", 4.0f, 4.0f, 4.0f);
		}

		// Let the "ILB_texPak01_decal002" (mos) material glow
		pMaterial = pRendererContext->GetMaterialManager().GetByName("Data\\Materials\\Dungeon\\ILB_texPak01_decal002.mat");
		if (pMaterial) {
			// Set emissive map color
			pMaterial->GetParameterManager().SetParameter3f("EmissiveMapColor", 2.0f, 2.0f, 2.0f);

			// Set glow
			pMaterial->GetParameterManager().SetParameter1f("Glow", 4.0f);
		}
	}

	// Call the post-load script function
	if (m_pScript)
		FuncScriptPtr<void>(m_pScript, "PostLoad").Call(Params<void>());

	// Create the interaction component
	m_pInteraction = new Interaction(*this);

	// Done
	return bResult;
}

/**
*  @brief
*    Shows a text
*/
void Application::ShowText(String sText, float fTimeout)
{
	// Call the show text script function
	if (m_pScript)
		FuncScriptPtr<void, String, float>(m_pScript, "ShowText").Call(Params<void, String, float>(sText, fTimeout));
}


//[-------------------------------------------------------]
//[ Private functions                                     ]
//[-------------------------------------------------------]
/**
*  @brief
*    Called on load progress
*/
void Application::OnLoadProgress(float fLoadProgress)
{
	// Time for an update?
	if ((fLoadProgress-m_fLoadProgress) >= 0.01f) {
		m_fLoadProgress = fLoadProgress;

		// Call the 'update'-function so we can see the progress within the load screen
		Update();
	}
}


//[-------------------------------------------------------]
//[ Protected virtual PLCore::ConsoleApplication functions ]
//[-------------------------------------------------------]
void Application::OnInitLog()
{
	// Sets the current directory to the path of the application executable
	GetApplicationContext().ChangeIntoAppDirectory();

	// Call base implementation
	BasicSceneApplication::OnInitLog();
}

void Application::OnInit()
{
	// Scene filename given?
	String sSceneFilename = m_cCommandLine.GetValue("Filename");
	if (!sSceneFilename.GetLength()) {
		// Load a default scene on start?
		if (DefaultScene.GetLength()) {
			// Set the default scene
			sSceneFilename = DefaultScene;
		}
	}

	// Is there a scene name given?
	if (sSceneFilename.GetLength()) {
		// Enable/disable edit mode
		SetEditModeEnabled(GetConfig().GetVar("DungeonConfig", "EditModeEnabled").GetBool());

		// Create the script instance
		m_pScript = ScriptManager::GetInstance()->CreateFromFile("Data/Scripts/Lua/Application.lua");
		if (m_pScript) {
			// Add the global variable "this" to the script so that it's able to access "this" RTTI class instance
			m_pScript->SetGlobalVariable("this", Var<Object*>(this));
		}

		// Load scene
		if (!LoadScene(sSceneFilename)) {
			// Present the user an sweet 'ERROR!!!'-message
			// [TODO] PLGui
			// pWindow->GetGui()->MessageBox("Error", "Failed to load the given scene, the program will terminate now.\nHave a look into the log for detailed information.");

			// Set exit code to error
			Exit(1);
		}
	} else {
		// No scene given
		Exit(1);
	}
}

void Application::OnDeInit()
{
	// Destroy the used script
	if (m_pScript) {
		delete m_pScript;
		m_pScript = nullptr;
	}

	// Destroy the interaction component
	if (m_pInteraction) {
		delete m_pInteraction;
		m_pInteraction = nullptr;
	}

	// Call base implementation
	BasicSceneApplication::OnDeInit();
}


//[-------------------------------------------------------]
//[ Protected virtual PLEngine::RenderApplication functions ]
//[-------------------------------------------------------]
bool Application::OnUpdate()
{
	// Call the update script function
	if (m_pScript)
		FuncScriptPtr<void>(m_pScript, "Update").Call(Params<void>());

	// Update the interaction application component
	if (m_pInteraction)
		m_pInteraction->Update();

	// Call base implementation
	return BasicSceneApplication::OnUpdate();
}


//[-------------------------------------------------------]
//[ Protected virtual PLEngine::SceneApplication functions ]
//[-------------------------------------------------------]
void Application::OnCreateRootScene()
{
	// Get the scene context
	SceneContext *pSceneContext = GetSceneContext();
	if (pSceneContext) {
		// First, create the scene root container which holds the scene container with our 'concrete' scene within it
		SceneContainer *pRootContainer = pSceneContext->GetRoot() ? static_cast<SceneContainer*>(pSceneContext->GetRoot()->Create("PLSound::SCSound", "RootScene", "SoundAPI=\"" + GetConfig().GetVar("DungeonConfig", "SoundAPI") + '"')) : nullptr;
		if (!pRootContainer)
			pRootContainer = pSceneContext->GetRoot() ? static_cast<SceneContainer*>(pSceneContext->GetRoot()->Create("PLScene::SceneContainer", "RootScene")) : nullptr;
		if (pRootContainer) {
			// Protect this important container!
			pRootContainer->SetProtected(true);

			// Create a scene container with our 'concrete scene'
			SceneNode *pSceneContainerNode = pRootContainer->Create("PLScene::SceneContainer", "Scene");
			if (pSceneContainerNode && pSceneContainerNode->IsInstanceOf("PLScene::SceneContainer")) {
				SceneContainer *pSceneContainer = static_cast<SceneContainer*>(pSceneContainerNode);

				// Protect this important container!
				pSceneContainer->SetProtected(true);

				// Connect event handler
				if (pSceneContainerNode->IsInstanceOf("PLScene::SceneContainer"))
					static_cast<SceneContainer*>(pSceneContainerNode)->EventLoadProgress.Connect(&EventHandlerLoadProgress);

				// Create the 'concrete scene'
				OnCreateScene(*pSceneContainer);
			}

			// Create scene node for engine information
			SceneNode *pSceneNode = pRootContainer->Create("PLScene::SNEngineInformation");
			if (pSceneNode)
				pSceneNode->SetActive(m_bEditModeEnabled);

			// Create console scene node - using the console command 'timescale <value>' we
			// can change the scene time (slowdown or accelerate)
			pSceneNode = pRootContainer->Create("PLScene::SNConsole");
			if (pSceneNode && pSceneNode->GetClass()->IsDerivedFrom("PLScene::SNConsoleBase")) {
				SNConsoleBase *pConsole = static_cast<SNConsoleBase*>(pSceneNode);

				// Register default commands
				pConsole->RegisterCommand(0,	"quit",			"",	"",	Functor<void, ConsoleCommand &>(&BasicSceneApplication::ConsoleCommandQuit, this));
				pConsole->RegisterCommand(0,	"exit",			"",	"",	Functor<void, ConsoleCommand &>(&BasicSceneApplication::ConsoleCommandQuit, this));
				pConsole->RegisterCommand(0,	"bye",			"",	"",	Functor<void, ConsoleCommand &>(&BasicSceneApplication::ConsoleCommandQuit, this));
				pConsole->RegisterCommand(0,	"logout",		"",	"",	Functor<void, ConsoleCommand &>(&BasicSceneApplication::ConsoleCommandQuit, this));

				// Edit commands
				pConsole->RegisterCommand(1,	"editdialog",	"",	"",	Functor<void, ConsoleCommand &>(&BasicSceneApplication::ConsoleCommandEditDialog, this));

				// Set active state
				pConsole->SetActive(m_bEditModeEnabled);
			}
		}

		// Set the root scene
		SetRootScene(pRootContainer);
	}
}


//[-------------------------------------------------------]
//[ Public virtual PLEngine::BasicSceneApplication functions ]
//[-------------------------------------------------------]
void Application::SetCamera(SNCamera *pCamera)
{
	// Call base implementation
	BasicSceneApplication::SetCamera(pCamera);

	// Make the current set camera to the listener of the sound manager
	SceneContainer *pRootScene = GetRootScene();	// The root scene is an instance of "PLSound::SCSound"
	if (pRootScene)
		pRootScene->SetAttribute("Listener", pCamera ? reinterpret_cast<SceneNode*>(pCamera)->GetAbsoluteName() : "");
}


//[-------------------------------------------------------]
//[ Private virtual PLEngine::BasicSceneApplication functions ]
//[-------------------------------------------------------]
void Application::OnCreateScene(SceneContainer &cContainer)
{
	// Set scene container flags
	cContainer.SetFlags(SceneNode::NoCulling | SceneNode::NoPause);

	// Setup scene surface painter
	SurfacePainter *pPainter = GetPainter();
	if (pPainter && pPainter->IsInstanceOf("PLScene::SPScene")) {
		SPScene *pSPScene = static_cast<SPScene*>(pPainter);
		pSPScene->SetRootContainer(cContainer.GetContainer());
		pSPScene->SetSceneContainer(&cContainer);
	}

	// Set scene container
	SetScene(&cContainer);
}
