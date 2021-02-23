#pragma once
#include "../CSC8503Common/GameWorld.h"
#include <set>

extern unsigned short players;

namespace NCL {
	namespace CSC8503 {
		class PhysicsSystem	{
		public:
			PhysicsSystem(GameWorld& g);
			~PhysicsSystem();

			void Clear();

			void Update(float dt);

			void UseGravity(bool state) {
				applyGravity = state;
			}

			void SetGlobalDamping(float d) {
				globalDamping = d;
			}

			void SetGravity(const Vector3& g);

			float GetLinearDamping() const {
				return linearDamping;
			}
			void SetLinearDamping(float d) {
				linearDamping = d;
			}
		protected:
			void BasicCollisionDetection();
			void BroadPhase();
			void NarrowPhase();

			void ClearForces();

			void IntegrateAccel(float dt);
			void IntegrateVelocity(float dt);

			void UpdateConstraints(float dt);

			void UpdateCollisionList();
			void UpdateObjectAABBs();

			void ImpulseResolveCollision(GameObject& a , GameObject&b, CollisionDetection::ContactPoint& p) const;
			void ResolveSpringCollision(GameObject& a, GameObject& b, CollisionDetection::ContactPoint& p) const;

			void CollectBonus(GameObject& a, GameObject& b) const;

			GameWorld& gameWorld;

			bool	applyGravity;
			Vector3 gravity;
			float	dTOffset;
			float	globalDamping;

			std::set<CollisionDetection::CollisionInfo> allCollisions;
			std::set<CollisionDetection::CollisionInfo> broadphaseCollisions;

			float linearDamping = 0.4f;
			bool useBroadPhase = true;
			int numCollisionFrames	= 5;
		};
	}
}

