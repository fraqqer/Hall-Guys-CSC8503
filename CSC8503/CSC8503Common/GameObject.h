#pragma once
#include "Transform.h"
#include "CollisionVolume.h"
#include "PhysicsObject.h"
#include "RenderObject.h"
#include <vector>

using std::vector;

namespace NCL {
	namespace CSC8503 {
		enum class Layers {
			LAYER_1 = 1 << 0,
			LAYER_2 = 1 << 1,
			LAYER_3 = 1 << 2
		};
		class GameObject	{
		public:
			GameObject(string name = "");
			~GameObject();

			void SetBoundingVolume(CollisionVolume* vol) {
				boundingVolume = vol;
			}

			const CollisionVolume* GetBoundingVolume() const {
				return boundingVolume;
			}

			vector<Layers>* GetLayers() const {
				return layers;
			}

			void SetLayers(vector<Layers>*& layers) {
				this->layers = layers;
			}

			bool IsActive() const {
				return isActive;
			}

			Transform& GetTransform() {
				return transform;
			}

			RenderObject* GetRenderObject() const {
				return renderObject;
			}

			PhysicsObject* GetPhysicsObject() const {
				return physicsObject;
			}

			void SetRenderObject(RenderObject* newObject) {
				renderObject = newObject;
			}

			void SetPhysicsObject(PhysicsObject* newObject) {
				physicsObject = newObject;
			}

			const string& GetName() const {
				return name;
			}

			void SetName(const std::string& name) {
				this->name = name;
			}

			virtual void OnCollisionBegin(GameObject* otherObject) {
				//std::cout << "OnCollisionBegin event occured!\n";
			}

			virtual void OnCollisionEnd(GameObject* otherObject) {
				//std::cout << "OnCollisionEnd event occured!\n";
			}

			bool GetBroadphaseAABB(Vector3&outsize) const;

			void UpdateBroadphaseAABB();

			void SetWorldID(int newID) {
				worldID = newID;
			}

			int		GetWorldID() const {
				return worldID;
			}

			bool win = false;
		protected:
			Transform			transform;

			CollisionVolume*	boundingVolume;
			PhysicsObject*		physicsObject;
			RenderObject*		renderObject;

			vector<Layers>* layers;
			bool	isActive;
			int		worldID;
			string	name;

			Vector3 broadphaseAABB;
		};
	}
}

