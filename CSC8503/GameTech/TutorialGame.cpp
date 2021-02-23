#include "TutorialGame.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"
#include "../CSC8503Common/PositionConstraint.h"
#include "../CSC8503Common/StateGameObject.h"
#include "../CSC8503Common/BehaviourNode.h"
#include "../CSC8503Common/BehaviourAction.h"
#include "../CSC8503Common/NavigationGrid.cpp"
#include "../CSC8503Common/BehaviourSequence.h"

using namespace NCL;
using namespace CSC8503;

TutorialGame::TutorialGame()	{
	world		= new GameWorld();
	renderer	= new GameTechRenderer(*world);
	physics		= new PhysicsSystem(*world);

	forceMagnitude	= 10.0f;
	useGravity		= true;
	inSelectionMode = false;

	Debug::SetRenderer(renderer);

	InitialiseAssets();
}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes, 
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void TutorialGame::InitialiseAssets() {
	auto loadFunc = [](const string& name, OGLMesh** into) {
		*into = new OGLMesh(name);
		(*into)->SetPrimitiveType(GeometryPrimitive::Triangles);
		(*into)->UploadToGPU();
	};

	loadFunc("cube.msh"		 , &cubeMesh);
	loadFunc("sphere.msh"	 , &sphereMesh);
	loadFunc("Male1.msh"	 , &charMeshA);
	loadFunc("courier.msh"	 , &charMeshB);
	loadFunc("security.msh"	 , &enemyMesh);
	loadFunc("coin.msh"		 , &bonusMesh);
	loadFunc("capsule.msh"	 , &capsuleMesh);

	basicTex	= (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png");
	basicShader = new OGLShader("GameTechVert.glsl", "GameTechFrag.glsl");

	InitCamera();
	InitWorld();
	InitGame();
}

TutorialGame::~TutorialGame()	{
	delete capsuleMesh;
	delete cubeMesh;
	delete sphereMesh;
	delete charMeshA;
	delete charMeshB;
	delete enemyMesh;
	delete bonusMesh;

	delete basicTex;
	delete basicShader;

	delete p1Char;
	delete p2Char;
	delete p3Char;
	delete p4Char;
	delete lockedObject;
	delete testStateObject;
	delete selectionObject;

	delete physics;
	delete renderer;
	delete world;
}

void TutorialGame::GeneratePath() {
	NavigationGrid grid("TestGrid1.txt");
	NavigationPath outPath;
	Vector3 startPos(40, 0, 0); // each x or . in text file represents "10" units so (80, 0, 10) becomes (8 columns, 0, 1 row)
	Vector3 endPos(100, 0, 60);

	bool found = grid.FindPath(startPos, endPos, outPath);

	Vector3 pos;
	while (outPath.PopWaypoint(pos)) {
		pos.z *= -1;
		testNodes.push_back(pos);
	}
}

void TutorialGame::AIBehaviourTree(float dt) {
	BehaviourAction* followPath = new BehaviourAction("Follow Path", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == BehaviourState::Initialise) {
			force = 400.0f;
			return BehaviourState::Ongoing;
		}
		else if (state == BehaviourState::Ongoing) {
			if (nodeIndex < testNodes.size()) {
				PhysicsObject* charPhysics = p2Char->GetPhysicsObject();
				Transform& transform = p2Char->GetTransform();
				Vector2 currentPos = Vector2(transform.GetPosition().x, transform.GetPosition().z);
				Vector2 nodePos = Vector2(testNodes[nodeIndex - 1].x, testNodes[nodeIndex - 1].z);

				distanceToNode = currentPos - nodePos;

				float positionThreshold = 0.1f;
				bool withinRangeOfTarget = abs(distanceToNode.x) < positionThreshold && abs(distanceToNode.y) < positionThreshold;

				if (!withinRangeOfTarget) {
					distanceToNode.Normalise();
					// close in the distance
					charPhysics->AddForce((Vector3(-distanceToNode.x, 0, -distanceToNode.y) * force) * dt);
					return BehaviourState::Success;
				}
				else {
					nodeIndex++;
					charPhysics->SetLinearVelocity(Vector3(0, 0, 0));
					return BehaviourState::Ongoing;
				}
			}
			return BehaviourState::Failure;
		}
		return state;
	});

	BehaviourSequence* rootSequence = new BehaviourSequence("Root Sequence");
	rootSequence->AddChild(followPath);

	rootSequence->Reset();
	BehaviourState state = BehaviourState::Ongoing;

	while (state == BehaviourState::Ongoing) {
		state = rootSequence->Execute(dt);
	}
}

void TutorialGame::UpdateGame(float dt) {
	if (!inSelectionMode) {
		world->GetMainCamera()->UpdateCamera(dt);
	}
	deductPoints -= dt;
	UpdateKeys(dt);

	// display score on screen
	for (int i = 0; i < players; i++) {
		std::string name = "Player " + std::to_string(i + 1);
		Debug::Print(name + ": " + std::to_string(world->playerScores[i]), Vector2(2, 5 * (i + 1)), Debug::RED);
	}

	// deduct points from players if it's been a second/more than a second
	if (deductPoints < 0.0f) {
		for (int i = 0; i < players; i++) {
			if (world->playerScores[0] <= 0) {
				gameOver = true;
				std::cout << "You lost! You scored: " + world->playerScores[0];
			}
			world->playerScores[i] -= 10;
		}
		deductPoints += 1.0f;
	}

	if (testStateObject) {
		testStateObject->Update(dt);
	}

	SelectObject();
	MoveSelectedObject();
	physics->Update(dt);

	if (p1Char != nullptr) {
		Vector3 objPos = p1Char->GetTransform().GetPosition();
		Vector3 camPos = objPos + lockedOffset;

		Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0,1,0));

		Matrix4 modelMat = temp.Inverse();

		Quaternion q(modelMat);
		Vector3 angles = q.ToEuler(); //nearly there now!

		world->GetMainCamera()->SetPosition(camPos);
		world->GetMainCamera()->SetPitch(angles.x);
		world->GetMainCamera()->SetYaw(angles.y);
	}

	if (p1Char->win) {
		std::cout << "You win! You scored: " + world->playerScores[0] << std::endl;
		gameOver = true;
	}

	world->UpdateWorld(dt);
	renderer->Update(dt);

	Debug::FlushRenderables(dt);
	renderer->Render();

	if (players > 1) {
		AIBehaviourTree(dt);

		if (p2Char->win) {
			std::cout << "You lost! You scored: " + world->playerScores[0];
			gameOver = true;
		}
	}
}

void TutorialGame::UpdateKeys(float dt) {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		InitWorld(); //We can reset the simulation at any time with F1
		selectionObject = nullptr;
		lockedObject	= nullptr;
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}

	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		world->ShuffleConstraints(false);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F8)) {
		world->ShuffleObjects(false);
	}

	PlayerControls(dt);

	if (!lockedObject) {
		DebugObjectMovement();
	}
}

void TutorialGame::PlayerControls(float dt) {
	Matrix4 view = world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld = view.Inverse();
	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis.Normalise();

	force = 2000.0f;
	float torqueForce = 200.0f;
	PhysicsObject* playerOne = p1Char->GetPhysicsObject();
	float modelDirection = p1Char->GetTransform().GetOrientation().ToEuler().y;

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT) || Window::GetKeyboard()->KeyDown(KeyboardKeys::A)) {
		Vector3 direction = -rightAxis * force;
		playerOne->AddForce(direction * dt);

		bool facingDirection = modelDirection >= 89 && modelDirection <= 91;	// greater than +89 but less than +91

		if (!facingDirection) {
			playerOne->AddTorque(Vector3(0, torqueForce, 0));
		}
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT) || Window::GetKeyboard()->KeyDown(KeyboardKeys::D)) {
		Vector3 direction = rightAxis * force;
		playerOne->AddForce(direction * dt);
		bool facingDirection = modelDirection >= -91 && modelDirection <= -89;

		if (!facingDirection) {
			playerOne->AddTorque(Vector3(0, -torqueForce, 0));
		}
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP) || Window::GetKeyboard()->KeyDown(KeyboardKeys::W)) {
		Vector3 direction = fwdAxis * force;
		playerOne->AddForce(direction * dt);

		bool facingDirection = modelDirection >= -1 && modelDirection <= 1;

		if (!facingDirection) {
			playerOne->AddTorque(Vector3(0, -torqueForce, 0));
		}
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN) || Window::GetKeyboard()->KeyDown(KeyboardKeys::S)) {
		Vector3 direction = -fwdAxis * force;
		playerOne->AddForce(direction * dt);

		bool facingDirection = abs(modelDirection >= 179);

		if (!facingDirection && modelDirection >= 0) {
			playerOne->AddTorque(Vector3(0, torqueForce, 0));
		}
		else if (!facingDirection && modelDirection < 0) {
			playerOne->AddTorque(Vector3(0, -torqueForce, 0));
		}
	}
	playerOne->SetAngularVelocity(Vector3(0, 0, 0));
}

void TutorialGame::InitGame() {
	physics->SetGravity(Vector3(0, -70.0f, 0));
	
	for (unsigned int i = 0; i < players; i++) {
		world->playerScores[i] = 1000;
	}

	// starting room
	AddFloorToWorld(Vector3(32,  0,   0), Vector3(30, 3, 30), TextureColour::WHITE);		// floor
	AddCubeToWorld(Vector3(64,  7,   0), Vector3(2,  5, 30), 0, TextureColour::RED);	// right wall
	AddCubeToWorld(Vector3(32,   7,  32), Vector3(30, 5,  2), 0, TextureColour::RED);	// back wall
	AddCubeToWorld(Vector3(0, 7,   0), Vector3(2,  5, 30), 0, TextureColour::RED);	// left wall
	AddCubeToWorld(Vector3(52, 7, -10), Vector3(10, 5, 2), 0, TextureColour::RED);
	AddCubeToWorld(Vector3(17, 7, -10), Vector3(15, 5, 2), 0, TextureColour::RED);
	AddCubeToWorld(Vector3(17, 7, -32), Vector3(15, 5, 2), 0, TextureColour::RED);
	AddCubeToWorld(Vector3(30, 7, -60), Vector3(2, 5, 30), 0, TextureColour::RED);
	AddSphereToWorld(Vector3(19, 7, -5), 2, 2.0f, false);
	AddSphereToWorld(Vector3(45, 7, -5), 2, 2.0f, false);
	AddCubeToWorld(Vector3(22, 7, -20), Vector3(3, 3, 3), 3.0f, TextureColour::RED);
	AddCubeToWorld(Vector3(17, 7, -20), Vector3(3, 3, 3), 3.0f, TextureColour::RED);
	AddBonusToWorld(Vector3(42, 8, 0));

	// big room
	AddFloorToWorld(Vector3(122, 0, -60), Vector3(90, 3, 30), TextureColour::WHITE);		// floor
	AddCubeToWorld(Vector3(122, 7, -90), Vector3(90, 5, 2), 0, TextureColour::RED);		// top wall
	AddCubeToWorld(Vector3(137, 7, -28), Vector3(75, 5, 2), 0, TextureColour::RED);		// bottom wall
	AddCubeToWorld(Vector3(212, 7, -50), Vector3(2, 5, 20), 0, TextureColour::RED);		// right bottom wall
	AddCubeToWorld(Vector3(212, 7, -85), Vector3(2, 5, 5), 0, TextureColour::RED);		// right top wall
	AddCubeToWorld(Vector3(152, 7, -48), Vector3(20, 5, 18), 0, TextureColour::RED);
	AddCubeToWorld(Vector3(187, 7, -70), Vector3(5, 5, 18), 0, TextureColour::RED);
	AddCubeToWorld(Vector3(72, 7, -35), Vector3(5, 5, 9), 0, TextureColour::RED);
	AddCubeToWorld(Vector3(77, 7, -60), Vector3(8, 5, 12), 0, TextureColour::RED);
	AddCapsuleToWorld(Vector3(130, 10, -80), 4, 2, 0.0f);
	AddCapsuleToWorld(Vector3(145, 10, -80), 4, 2, 0.0f);
	AddCubeToWorld(Vector3(47, 3.01f, -60), Vector3(5, 0, 5), 0, TextureColour::PURPLE);
	SetupBridge();

	finish = AddFloorToWorld(Vector3(330, -60, -60), Vector3(50, 3, 20), TextureColour::BLUE);

	switch (players) {
	case 2:
		p1Char = AddPlayerToWorld(Vector3(32, 7, 0), "player1");
		p2Char = AddPlayerToWorld(Vector3(36, 7, 0), "player2");
		break;
	case 3:
		p1Char = AddPlayerToWorld(Vector3(32, 7, 0), "player1");
		p2Char = AddPlayerToWorld(Vector3(36, 7, 0), "player2");
		p3Char = AddPlayerToWorld(Vector3(40, 7, 0), "player3");
		break;
	case 4:
		p1Char = AddPlayerToWorld(Vector3(32, 7, 0), "player1");
		p2Char = AddPlayerToWorld(Vector3(36, 7, 0), "player2");
		p3Char = AddPlayerToWorld(Vector3(40, 7, 0), "player3");
		p4Char = AddPlayerToWorld(Vector3(28, 7, 0), "player4");
		break;
	default:
		p1Char = AddPlayerToWorld(Vector3(32, 7, 0), "player1");
		break;
	}

	if (players > 1) {
		GeneratePath();
	}
}

void TutorialGame::DebugObjectMovement() {
//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject) {
		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
		}
	}

}

void TutorialGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.1f);
	world->GetMainCamera()->SetFarPlane(500.0f);
	world->GetMainCamera()->SetPitch(-15.0f);
	world->GetMainCamera()->SetYaw(315.0f);
	world->GetMainCamera()->SetPosition(Vector3(0, 0, 0));
	lockedObject = nullptr;
}

void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();
	physics->UseGravity(useGravity);

	// testStateObject = AddStateObjectToWorld(Vector3(0.0f, 10.0f, 0.0f));
}

void TutorialGame::SetupBridge() {
	Vector3 cubeSize = Vector3(4, 4, 4);

	float invCubeMass = 2.0f; // how heavy the middle pieces are
	int numLinks = 3;
	float maxDistance = 7.0f; // constraint difference
	float cubeDistance = 2.0f; // distance between links

	Vector3 startPos = Vector3(219, -2, -75);
	
	GameObject* start = AddCubeToWorld(startPos + Vector3(0, 0, 0), cubeSize, 0);
	GameObject* end = AddCubeToWorld(startPos + Vector3(60, -60, 0), cubeSize, 0);

	float yDiff = startPos.y - end->GetTransform().GetPosition().y;

	GameObject* previous = start;
	for (int i = 0; i < numLinks; ++i) {
		GameObject* block = AddCubeToWorld(startPos + Vector3((i + 1) * cubeDistance, yDiff * (i / numLinks), 0), cubeSize, invCubeMass);
		PositionConstraint* constraint = new PositionConstraint(previous, block, maxDistance);
		world->AddConstraint(constraint);
		previous = block;
	}

	PositionConstraint* constraint = new PositionConstraint(previous, end, maxDistance);
	world->AddConstraint(constraint);
}

/*

A single function to add a large immoveable cube to the bottom of our world

*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position, const Vector3& size, TextureColour textureID) {
	GameObject* floor = new GameObject("floor");

	Vector3 floorSize	= size;
	AABBVolume* volume	= new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(position);
	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, nullptr, basicShader));

	switch (textureID) {
	case TextureColour::RED:
		floor->GetRenderObject()->SetColour(Vector4(1, 0, 0, 1));
		break;
	case TextureColour::BLUE:
		floor->SetName("finish");
		floor->GetRenderObject()->SetColour(Vector4(0, 0, 1, 1));
		break;
	case TextureColour::WHITE:
		floor->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
		break;
	}

	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));
	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();
	world->AddGameObject(floor);

	return floor;
}

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple' 
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass, bool solid) {
	GameObject* sphere = new GameObject();

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->GetRenderObject()->SetColour(Vector4(1, 0, 0, 1));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	solid ? sphere->GetPhysicsObject()->InitSolidSphereInertia() : sphere->GetPhysicsObject()->InitHollowSphereInertia();

	world->AddGameObject(sphere);

	return sphere;
}

GameObject* TutorialGame::AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass) {
	GameObject* capsule = new GameObject();

	CapsuleVolume* volume = new CapsuleVolume(halfHeight, radius);
	capsule->SetBoundingVolume((CollisionVolume*)volume);

	capsule->GetTransform()
		.SetScale(Vector3(radius* 2, halfHeight, radius * 2))
		.SetPosition(position);

	capsule->SetRenderObject(new RenderObject(&capsule->GetTransform(), capsuleMesh, basicTex, basicShader));
	capsule->SetPhysicsObject(new PhysicsObject(&capsule->GetTransform(), capsule->GetBoundingVolume()));

	capsule->GetPhysicsObject()->SetInverseMass(inverseMass);
	capsule->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(capsule);

	return capsule;

}

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass, TextureColour textureID) {
	GameObject* cube = new GameObject();
	AABBVolume* volume = new AABBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, nullptr, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));
	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();
	switch (textureID) {
	case TextureColour::RED:
		cube->GetRenderObject()->SetColour(Vector4(1, 0, 0, 1));
		break;
	case TextureColour::WHITE:
		cube->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
		break;
	case TextureColour::PURPLE:
		cube->SetName("slippery");
		cube->GetRenderObject()->SetColour(Vector4(0.5f, 0, 0.5f, 1));
		break;
	case TextureColour::BLUE:
		cube->GetRenderObject()->SetColour(Vector4(0, 0, 1, 1));
		break;
	}

	world->AddGameObject(cube);
	return cube;
}

void TutorialGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddSphereToWorld(position, radius, 1.0f);
		}
	}
	AddFloorToWorld(Vector3(0, -2, 0), Vector3(100, 2, 100));
}

void TutorialGame::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	Vector3 cubeDims = Vector3(1, 1, 1);
	float sphereRadius = 1.0f;
	float halfHeight = 2.0f;

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);

			switch (rand() % 3)
			{
			case 0:
				AddCubeToWorld(position, cubeDims);
				break;
			case 1: 
				AddSphereToWorld(position, sphereRadius);
				break;
			case 2:
				AddCapsuleToWorld(position, halfHeight, sphereRadius);
				break;
			}
		}
	}
}

void TutorialGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols+1; ++x) {
		for (int z = 1; z < numRows+1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, 1.0f);
		}
	}
}

void TutorialGame::InitDefaultFloor() {
	AddFloorToWorld(Vector3(0, -2, 0), Vector3(100, 2, 100));
}

void TutorialGame::InitGameExamples() {
	AddPlayerToWorld(Vector3(0, 5, 0), "player1");
	AddEnemyToWorld(Vector3(5, 5, 0));
	AddBonusToWorld(Vector3(10, 5, 0));
}

GameObject* TutorialGame::AddPlayerToWorld(const Vector3& position, const std::string& name) {
	float meshSize = 3.0f;
	float inverseMass = 3.0f;

	GameObject* character = new GameObject(name);

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.85f, 0.3f) * meshSize);

	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), charMeshA, nullptr, basicShader));

	switch (name[6]) {
	case '1':
		character->GetRenderObject()->SetColour(Vector4(1, 0, 0, 1));
		break;
	case '2':
		character->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
		break;
	case '3':
		character->GetRenderObject()->SetColour(Vector4(0, 0, 1, 1));
		break;
	case '4':
		character->GetRenderObject()->SetColour(Vector4(1, 0, 1, 1));
		break;
	}

	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSolidSphereInertia();

	world->AddGameObject(character);
	return character;
}

GameObject* TutorialGame::AddEnemyToWorld(const Vector3& position) {
	float meshSize		= 3.0f;
	float inverseMass	= 0.5f;

	GameObject* character = new GameObject();

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), enemyMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSolidSphereInertia();

	world->AddGameObject(character);

	return character;
}

GameObject* TutorialGame::AddBonusToWorld(const Vector3& position) {
	GameObject* bonus = new GameObject("bonus");

	SphereVolume* volume = new SphereVolume(0.25f);
	bonus->SetBoundingVolume((CollisionVolume*)volume);
	bonus->GetTransform()
		.SetScale(Vector3(0.25, 0.25, 0.25))
		.SetPosition(position);

	bonus->SetRenderObject(new RenderObject(&bonus->GetTransform(), bonusMesh, nullptr, basicShader));
	bonus->GetRenderObject()->SetColour(Vector4(1, 0, 0, 1));

	bonus->SetPhysicsObject(new PhysicsObject(&bonus->GetTransform(), bonus->GetBoundingVolume()));
	bonus->GetPhysicsObject()->SetInverseMass(0.0f);
	bonus->GetPhysicsObject()->InitSolidSphereInertia();

	world->AddGameObject(bonus);

	return bonus;
}

StateGameObject* TutorialGame::AddStateObjectToWorld(const Vector3& position) {
	StateGameObject* stateObj = new StateGameObject();

	SphereVolume* volume = new SphereVolume(0.25f);
	stateObj->SetBoundingVolume((CollisionVolume*)volume);
	stateObj->GetTransform()
		.SetScale(Vector3(0.25, 0.25, 0.25))
		.SetPosition(position);

	stateObj->SetRenderObject(new RenderObject(&stateObj->GetTransform(), bonusMesh, nullptr, basicShader));
	stateObj->SetPhysicsObject(new PhysicsObject(&stateObj->GetTransform(), stateObj->GetBoundingVolume()));

	stateObj->GetPhysicsObject()->SetInverseMass(1.0f);
	stateObj->GetPhysicsObject()->InitSolidSphereInertia();

	world->AddGameObject(stateObj);

	return stateObj;
}

/*

Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be 
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around. 

*/
bool TutorialGame::SelectObject() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Q)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
	}
	if (inSelectionMode) {
		renderer->DrawString("Press Q to change to camera mode!", Vector2(5, 85));

		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {
			if (selectionObject) {	//set colour to deselected;
				selectionObject->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
				selectionObject = nullptr;
				lockedObject	= nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

			RayCollision closestCollision;
			if (world->Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;
				selectionObject->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));

				Transform& t = selectionObject->GetTransform();
				Vector3 pos = t.GetPosition();
				Quaternion rot = t.GetOrientation();

				renderer->DrawString("Position: [" + std::to_string(pos.x) + ", " + std::to_string(pos.y) + ", " + std::to_string(pos.z) + "]", Vector2(20, 5));
				renderer->DrawString("Orientation: [" + std::to_string(rot.x) + ", " + std::to_string(rot.y) + ", " + std::to_string(rot.z) + ", " + std::to_string(rot.w) + "]",
					Vector2(20, 10));
				return true;
			}
			else {
				return false;
			}
		}
	}
	else {
		renderer->DrawString("Press Q to change to select mode!", Vector2(5, 85));
	}

	if (lockedObject) {
		renderer->DrawString("Press L to unlock object!", Vector2(5, 80));
	}

	else if(selectionObject){
		renderer->DrawString("Press L to lock selected object object!", Vector2(5, 80));
	}

	if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::L)) {
		if (selectionObject) {
			if (lockedObject == selectionObject) {
				lockedObject = nullptr;
			}
			else {
				lockedObject = selectionObject;
			}
		}

	}

	return false;
}

/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/
void TutorialGame::MoveSelectedObject() {
	renderer->DrawString("Click Force:" + std::to_string(forceMagnitude), Vector2(10, 20)); // Draw debug text at (10, 20)
	forceMagnitude += Window::GetMouse()->GetWheelMovement() * 100.0f;

	if (!selectionObject)
		return; // We haven't selected anything

	// Push the selected object!
	if (Window::GetMouse()->ButtonPressed(MouseButtons::RIGHT)) {
		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());
		
		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, true)) {
			if (closestCollision.node == selectionObject) {
				selectionObject->GetPhysicsObject()->AddForceAtPosition(ray.GetDirection() * forceMagnitude, closestCollision.collidedAt);
			}
		}
	}
}