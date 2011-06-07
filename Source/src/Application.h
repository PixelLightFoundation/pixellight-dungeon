/*********************************************************\
 *  File: Application.h                                  *
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


#ifndef __DUNGEON_APPLICATION_H__
#define __DUNGEON_APPLICATION_H__
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <PLEngine/Application/BasicSceneApplication.h>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace PLScript {
	class Script;
}
namespace PLScene {
	class SceneRendererPass;
}
class Interaction;


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    Application class
*/
class Application : public PLEngine::BasicSceneApplication {


	//[-------------------------------------------------------]
	//[ RTTI interface                                        ]
	//[-------------------------------------------------------]
	pl_class(pl_rtti_export, Application, "", PLEngine::BasicSceneApplication, "Application class")
		// Constructors
		pl_constructor_0(DefaultConstructor,	"Default constructor",	"")
	pl_class_end


	//[-------------------------------------------------------]
	//[ Public static data                                    ]
	//[-------------------------------------------------------]
	public:
		static const PLGeneral::String DefaultScene;	/**< The used default scene loaded at start, if empty a file dialog will appear */


	//[-------------------------------------------------------]
	//[ Public functions                                      ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*/
		Application();

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~Application();

		/**
		*  @brief
		*    Returns whether or not the application runs within the expert mode
		*
		*  @return
		*    'true' if the application runs within the expert mode, else 'false' (no additional help texts)
		*/
		bool IsExpertMode() const;

		/**
		*  @brief
		*    Returns whether or not the application runs within the repeat mode
		*
		*  @return
		*    'true' if the application runs within the repeat mode ("movie -> making of -> movie" instead of "movie -> making of -> interactive"), else 'false'
		*/
		bool IsRepeatMode() const;

		/**
		*  @brief
		*    Returns the interaction component
		*
		*  @return
		*    The interaction component, can be a null pointer
		*/
		Interaction *GetInteraction();

		/**
		*  @brief
		*    Loads a scene
		*
		*  @param[in] sFilename
		*    Filename of the scene to load
		*
		*  @return
		*    'true' if all went fine, else 'false'
		*
		*  @note
		*    - Similar to PLEngine::BasicSceneApplication::LoadScene, but sets the base path the scene is in, as the new current system directory
		*/
		bool LoadScene(const PLGeneral::String &sFilename);

		/**
		*  @brief
		*    Shows a text
		*
		*  @param[in] sText
		*    Text to show
		*  @param[in] fTimeout
		*    Timeout (in seconds)
		*/
		void ShowText(PLGeneral::String sText, float fTimeout = 5.0f);


	//[-------------------------------------------------------]
	//[ Private functions                                     ]
	//[-------------------------------------------------------]
	private:
		/**
		*  @brief
		*    Called on load progress
		*
		*  @param[in] fLoadProgress
		*    Load progress (0.0-1.0)
		*/
		void OnLoadProgress(float fLoadProgress);


	//[-------------------------------------------------------]
	//[ Protected virtual PLCore::ConsoleApplication functions ]
	//[-------------------------------------------------------]
	protected:
		virtual void OnInitLog();
		virtual void OnInit();
		virtual void OnDeInit();


	//[-------------------------------------------------------]
	//[ Protected virtual PLEngine::RenderApplication functions ]
	//[-------------------------------------------------------]
	protected:
		virtual bool OnUpdate();


	//[-------------------------------------------------------]
	//[ Protected virtual PLEngine::SceneApplication functions ]
	//[-------------------------------------------------------]
	protected:
		virtual void OnCreateRootScene();


	//[-------------------------------------------------------]
	//[ Public virtual PLEngine::BasicSceneApplication functions ]
	//[-------------------------------------------------------]
	public:
		virtual void SetCamera(PLScene::SNCamera *pCamera);


	//[-------------------------------------------------------]
	//[ Private virtual PLEngine::BasicSceneApplication functions ]
	//[-------------------------------------------------------]
	private:
		virtual void OnCreateScene(PLScene::SceneContainer &cContainer);


	//[-------------------------------------------------------]
	//[ Private event handlers                                ]
	//[-------------------------------------------------------]
	private:
		PLCore::EventHandler<float> EventHandlerLoadProgress;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		PLScript::Script  *m_pScript;						/**< The used script, can be a null pointer */
		Interaction		  *m_pInteraction;					/**< Interaction application component, can be a null pointer */
		float			   m_fLoadProgress;					/**< Load progress */
		PLGeneral::String  m_sCurrentSceneBaseDirectory;	/**< Base directory of the currently loaded scene */


};


#endif // __DUNGEON_APPLICATION_H__
