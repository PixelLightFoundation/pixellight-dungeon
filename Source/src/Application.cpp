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
#include <PLCore/File/Url.h>
#include <PLCore/Base/Class.h>
#include <PLCore/Script/Script.h>
#include <PLCore/System/System.h>
#include <PLCore/Tools/Timing.h>
#include <PLCore/Tools/Localization.h>
#include <PLCore/Tools/LoadableManager.h>
#include <PLGui/Gui/Gui.h>
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
#include <PLPhysics/Body.h>
#include <PLPhysics/SceneNodeModifiers/SNMPhysicsBodyBox.h>
#include <PLPhysics/SceneNodes/SNPhysicsMouseInteraction.h>
#include <PLEngine/Gui/RenderWindow.h>
#include "Application.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
using namespace PLCore;
using namespace PLMath;
using namespace PLGui;
using namespace PLRenderer;
using namespace PLScene;
using namespace PLEngine;
using namespace PLPhysics;


//[-------------------------------------------------------]
//[ RTTI interface                                        ]
//[-------------------------------------------------------]
pl_implement_class(Application)


//[-------------------------------------------------------]
//[ Public functions                                      ]
//[-------------------------------------------------------]
/**
*  @brief
*    Constructor
*/
Application::Application() : ScriptApplication("Data/Scripts/Lua/Main.lua", "Dungeon", PLT("PixelLight dungeon demo"), System::GetInstance()->GetDataDirName("PixelLight")),
	SlotOnLoadProgress(this),
	m_pIngameGui(nullptr),
	m_pCamcorder(new Camcorder(*this)),
	m_fLoadProgress(0.0f),
	m_fMousePickingPullAnimation(0.0f)
{
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
	// Destroy the ingame GUI component
	if (m_pIngameGui)
		delete m_pIngameGui;

	// Destroy the camcorder component
	delete m_pCamcorder;
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
*    Returns whether or not this is an internal release
*/
bool Application::IsInternalRelease() const
{
	#ifdef INTERNALRELEASE
		return true;
	#else
		return false;
	#endif
}

/**
*  @brief
*    Returns the ingame GUI interaction component instance
*/
IngameGui &Application::GetIngameGui() const
{
	// When this method is used, the pointer is valid
	return *m_pIngameGui;
}

/**
*  @brief
*    Returns the camcorder component instance
*/
Camcorder &Application::GetCamcorder() const
{
	return *m_pCamcorder;
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

/**
*  @brief
*    Updates the mouse picking pull animation
*/
void Application::UpdateMousePickingPullAnimation()
{
	// Get the current time difference
	const float fTimeDiff = Timing::GetInstance()->GetTimeDifference();

	// Update the mouse picking pull animation
	m_fMousePickingPullAnimation += fTimeDiff*5;

	// Get the scene
	SceneContainer *pSceneContainer = GetScene();
	if (pSceneContainer) {
		// Get the "PLPhysics::SNPhysicsMouseInteraction" instance
		SceneNode *pSceneNode = pSceneContainer->GetByName("Container.SNPhysicsMouseInteraction");
		if (pSceneNode && pSceneNode->IsInstanceOf("PLPhysics::SNPhysicsMouseInteraction")) {
			SNPhysicsMouseInteraction *pSNPhysicsMouseInteraction = static_cast<SNPhysicsMouseInteraction*>(pSceneNode);

			// Get the currently set camera scene node
			SceneNode *pCameraSceneNode = reinterpret_cast<SceneNode*>(GetCamera());
			if (pCameraSceneNode) {
				// Is picking currently performed?
				Vector2i vMousePos;
				if (pSNPhysicsMouseInteraction->IsPicking(&vMousePos)) {
					// Update "PLPostProcessEffects::SNMPostProcessPull" modifier
					SceneNodeModifier *pSceneNodeModifier = pCameraSceneNode->GetModifier("PLPostProcessEffects::SNMPostProcessPull");
					if (!pSceneNodeModifier)
						pSceneNodeModifier = pCameraSceneNode->AddModifier("PLPostProcessEffects::SNMPostProcessPull");
					if (pSceneNodeModifier) {
						// Get the main window of the application
						Widget *pWidget = GetMainWindow();
						if (pWidget && pWidget->GetContentWidget()) {
							pSceneNodeModifier->SetAttribute("WarpPoint",	  String::Format("%d %d", vMousePos.x, pWidget->GetContentWidget()->GetSize().y-vMousePos.y));
							pSceneNodeModifier->SetAttribute("WarpScale",	  -5.0f  + Math::Sin(m_fMousePickingPullAnimation)*Math::Cos(m_fMousePickingPullAnimation/4)*10.0f);
							pSceneNodeModifier->SetAttribute("WarpDimension", 150.0f + Math::Cos(m_fMousePickingPullAnimation)*Math::Sin(m_fMousePickingPullAnimation/6)*60.0f);
						}
					}
				} else {
					// Remove "PLPostProcessEffects::SNMPostProcessPull" modifier
					pCameraSceneNode->RemoveModifier("PLPostProcessEffects::SNMPostProcessPull");
				}
			}
		}
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
	ScriptApplication::OnInitLog();
}

void Application::OnInit()
{
	// Create the ingame GUI component
	m_pIngameGui = new IngameGui(*this);

	// Call base implementation
	ScriptApplication::OnInit();

	// Scene filename given as command line parameter?
	String sSceneFilename = m_cCommandLine.GetValue("Filename");
	if (!sSceneFilename.GetLength()) {
		// Ask the script for a scene filename
		if (m_pScript)
			sSceneFilename = m_pScript->GetGlobalVariable("SceneFilename");
	}

	// Is there a scene name given?
	if (sSceneFilename.GetLength()) {
		// Enable/disable edit mode
		SetEditModeEnabled(GetConfig().GetVar("DungeonConfig", "EditModeEnabled").GetBool());

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
	// Destroy the ingame GUI component
	if (m_pIngameGui) {
		delete m_pIngameGui;
		m_pIngameGui = nullptr;
	}

	// Call base implementation
	ScriptApplication::OnDeInit();
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
					static_cast<SceneContainer*>(pSceneContainerNode)->SignalLoadProgress.Connect(SlotOnLoadProgress);

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
				pConsole->RegisterCommand(0,	"quit",			"",	"",	Functor<void, ConsoleCommand &>(&ScriptApplication::ConsoleCommandQuit, this));
				pConsole->RegisterCommand(0,	"exit",			"",	"",	Functor<void, ConsoleCommand &>(&ScriptApplication::ConsoleCommandQuit, this));
				pConsole->RegisterCommand(0,	"bye",			"",	"",	Functor<void, ConsoleCommand &>(&ScriptApplication::ConsoleCommandQuit, this));
				pConsole->RegisterCommand(0,	"logout",		"",	"",	Functor<void, ConsoleCommand &>(&ScriptApplication::ConsoleCommandQuit, this));

				// Edit commands
				pConsole->RegisterCommand(1,	"editdialog",	"",	"",	Functor<void, ConsoleCommand &>(&ScriptApplication::ConsoleCommandEditDialog, this));

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
bool Application::LoadScene(String sFilename)
{
	// Reset the current load progress
	m_fLoadProgress = 0.0f;

	// Call base implementation
	const bool bResult = ScriptApplication::LoadScene(sFilename);

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

	// Done
	return bResult;
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
