#pragma once
#include "GameTechRenderer.h"
#include "../CSC8503Common/PhysicsSystem.h"
#include "../CSC8503Common/StateGameObject.h"
#include <future>
#include <thread>
#include <mutex>

extern unsigned short players;

namespace NCL {
	namespace CSC8503 {
		enum class TextureColour {
			NONE = 0,
			RED,
			WHITE,
			PURPLE,
			BLUE
		};

		class TutorialGame		{
		public:
			TutorialGame();
			~TutorialGame();

			virtual void UpdateGame(float dt);
			bool gameOver = false;
		protected:
			void InitialiseAssets();
			void InitCamera();
			void UpdateKeys(float dt);
			void InitWorld();
			void InitGameExamples();
			void InitGame();
			void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			void InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing);
			void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);
			void InitDefaultFloor();
			void SetupBridge();
	
			bool SelectObject();
			void MoveSelectedObject();
			void DebugObjectMovement();
			void PlayerControls(float dt);

			GameObject* AddFloorToWorld(const Vector3& position, const Vector3& size, TextureColour textureID = (TextureColour)0);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f, bool solid = true);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f, TextureColour textureID = (TextureColour)0);
			
			GameObject* AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass = 10.0f);

			GameObject* AddPlayerToWorld(const Vector3& position, const std::string& name);
			GameObject* AddEnemyToWorld(const Vector3& position);
			GameObject* AddBonusToWorld(const Vector3& position);

			GameObject* finish;

			StateGameObject* AddStateObjectToWorld(const Vector3& position);

			StateGameObject* testStateObject = nullptr;

			GameTechRenderer*	renderer;
			PhysicsSystem*		physics;
			GameWorld*			world;
			vector<Vector3> testNodes;

			std::mutex mtx;

			float		deductPoints	= 1.0f;
			float		forceMagnitude;
			float		force;

			unsigned int	nodeIndex = 1;

			bool useGravity			= true;
			bool inSelectionMode;

			GameObject* selectionObject = nullptr;

			OGLMesh*	capsuleMesh = nullptr;
			OGLMesh*	cubeMesh	= nullptr;
			OGLMesh*	sphereMesh	= nullptr;
			OGLTexture* basicTex	= nullptr;
			OGLShader*	basicShader = nullptr;

			//Coursework Meshes
			OGLMesh*	charMeshA	= nullptr;
			OGLMesh*	charMeshB	= nullptr;
			OGLMesh*	enemyMesh	= nullptr;
			OGLMesh*	bonusMesh	= nullptr;

			//Coursework Additional functionality	
			GameObject* lockedObject	= nullptr;
			Vector3 lockedOffset		= Vector3(0, 40, 20);
			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}

			// Game specific functionality
			GameObject* p1Char = nullptr;
			GameObject* p2Char = nullptr;
			GameObject* p3Char = nullptr;
			GameObject* p4Char = nullptr;

			bool distanceSet = false;
			Vector2 distanceToNode;
			void AIBehaviourTree(float dt);
			void GeneratePath();
		};
	}
}

